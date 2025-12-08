//////////////////////////////////////////////////////////////////////////////
// reverb.c - Implementação do Efeito Reverb
//////////////////////////////////////////////////////////////////////////////

#include "reverb.h"
#include <math.h>

// Taxa de amostragem
#define FS_FLOAT 48000.0f

// Buffers na memória de efeitos
#pragma DATA_SECTION(combBuffer, "effectsMem")
#pragma DATA_ALIGN(combBuffer, 4)
static Int16 combBuffer[REVERB_NUM_COMBS * REVERB_COMB_CHUNK];

#pragma DATA_SECTION(apBuffer, "effectsMem")
#pragma DATA_ALIGN(apBuffer, 4)
static Int16 apBuffer[REVERB_NUM_ALLPASSES * REVERB_AP_CHUNK];

// Variáveis globais
Reverb       g_reverb;
ReverbPreset g_reverbPreset = REVERB_PRESET_HALL;

// -------------------------------------------------------------------------
// Funções auxiliares
// -------------------------------------------------------------------------
static inline Int16 sat16(Int32 x)
{
    if (x > 32767)  return 32767;
    if (x < -32768) return -32768;
    return (Int16)x;
}

static Int16 floatToQ15(float x)
{
    if (x >= 1.0f)  return 32767;
    if (x <= -1.0f) return -32768;
    return (Int16)(x * 32768.0f);
}

static Uint16 msToSamples(float ms)
{
    float samples = ms * (FS_FLOAT / 1000.0f);
    return (Uint16)samples;
}

typedef struct {
    float comb_ms[REVERB_NUM_COMBS];
    float ap_ms[REVERB_NUM_ALLPASSES];
    float comb_gains[REVERB_NUM_COMBS];
    float ap_gains[REVERB_NUM_ALLPASSES];
    float wet_gain;   // ganho da parte reverberada (0..1)
} ReverbPresetCfg;

static const ReverbPresetCfg REVERB_PRESETS[REVERB_PRESET_COUNT] = {
    // REVERB_PRESET_HALL  ("REV-HALL")
    {
        // combs_ms
        { 26.84f, 28.02f, 45.82f, 33.63f },
        // aps_ms
        { 2.35f,  6.90f },
        // combs_gains
        { 0.758f, 0.854f, 0.796f, 0.825f },
        // aps_gains
        { 0.653f, 0.659f },
        // wet_gain
        0.4f
    },

    // REVERB_PRESET_ROOM ("REV-ROOM")
    {
        { 419.0f, 359.0f, 251.0f, 467.0f },
        { 7.06f,   6.46f },
        { 0.50f,   0.48f, 0.56f, 0.44f },
        { 0.716f,  0.613f },
        0.2f
    },

    // REVERB_PRESET_STAGE ("REV-STAGE")
    {
        { 46.27f, 39.96f, 28.03f, 51.85f },
        { 3.50f,  1.20f },
        { 0.758f, 0.854f, 0.796f, 0.825f },
        { 0.70f,  0.70f },
        0.5f
    }
};

// -------------------------------------------------------------------------
// Inicialização do Reverb (usa o preset atual g_reverbPreset)
// -------------------------------------------------------------------------
void initReverb(void)
{
    int i, j;

    if (g_reverbPreset >= REVERB_PRESET_COUNT) {
        g_reverbPreset = REVERB_PRESET_HALL;
    }

    const ReverbPresetCfg* p = &REVERB_PRESETS[g_reverbPreset];

    // Mix Dry/Wet (dry = 1 - wet)
    float dry = 1.0f - p->wet_gain;
    g_reverb.wet_gain_Q15 = floatToQ15(p->wet_gain);
    g_reverb.dry_gain_Q15 = floatToQ15(dry);

    // ---------------- COMB FILTERS (paralelo) ----------------
    for (i = 0; i < REVERB_NUM_COMBS; i++) {
        CombFilter* c = &g_reverb.comb[i];
        c->buffer = &combBuffer[i * REVERB_COMB_CHUNK];

        Uint16 samples = msToSamples(p->comb_ms[i]);
        if (samples < 10)                 samples = 10;
        if (samples > REVERB_COMB_CHUNK)  samples = REVERB_COMB_CHUNK;

        c->delay_samples = samples;
        c->gain_Q15      = floatToQ15(p->comb_gains[i]);
        c->ptr           = 0;

        for (j = 0; j < REVERB_COMB_CHUNK; j++) {
            c->buffer[j] = 0;
        }
    }

    // ---------------- ALL-PASS (série) ----------------
    for (i = 0; i < REVERB_NUM_ALLPASSES; i++) {
        AllPassFilter* ap = &g_reverb.allpass[i];
        ap->buffer = &apBuffer[i * REVERB_AP_CHUNK];

        Uint16 samples = msToSamples(p->ap_ms[i]);
        if (samples < 10)               samples = 10;
        if (samples > REVERB_AP_CHUNK)  samples = REVERB_AP_CHUNK;

        ap->delay_samples = samples;
        ap->gain_Q15      = floatToQ15(p->ap_gains[i]);
        ap->ptr           = 0;

        for (j = 0; j < REVERB_AP_CHUNK; j++) {
            ap->buffer[j] = 0;
        }
    }
}

// -------------------------------------------------------------------------
// Processamento All-Pass
// -------------------------------------------------------------------------
static Int16 processAllPass(Int16 input, AllPassFilter* apf)
{
    Int16 delayed = apf->buffer[apf->ptr];

    // v[n] = x[n] + g * d[n]
    Int32 vn = (Int32)input + (((Int32)apf->gain_Q15 * (Int32)delayed) >> 15);

    // y[n] = -g * v[n] + d[n]
    Int32 output = -(((Int32)apf->gain_Q15 * vn) >> 15) + (Int32)delayed;

    // Salva v[n] no buffer
    apf->buffer[apf->ptr] = sat16(vn);

    apf->ptr++;
    if (apf->ptr >= apf->delay_samples) {
        apf->ptr = 0;
    }

    return sat16(output);
}

// -------------------------------------------------------------------------
// Processamento do Reverb (amostra por amostra)
// -------------------------------------------------------------------------
static Int16 processReverbSample(Int16 input)
{
    int i;
    Int32 accComb = 0;

    // Atenua entrada para evitar overflow
    Int16 attInput = input >> 2;

    // Combs em paralelo
    for (i = 0; i < REVERB_NUM_COMBS; i++) {
        CombFilter* c = &g_reverb.comb[i];
        Int16 delayed = c->buffer[c->ptr];
        accComb += (Int32)delayed;

        // Feedback: Input + Gain * Delayed
        Int32 fb = (Int32)attInput +
                   (((Int32)c->gain_Q15 * (Int32)delayed) >> 15);
        c->buffer[c->ptr] = sat16(fb);

        c->ptr++;
        if (c->ptr >= c->delay_samples) {
            c->ptr = 0;
        }
    }

    // Soma dos combs
    Int16 combOut = sat16(accComb >> 1);

    // All-pass em série
    Int16 apSignal = combOut;
    for (i = 0; i < REVERB_NUM_ALLPASSES; i++) {
        apSignal = processAllPass(apSignal, &g_reverb.allpass[i]);
    }

    // Mix Dry/Wet
    Int32 dryPart = ((Int32)g_reverb.dry_gain_Q15 * (Int32)input) >> 15;
    Int32 wetPart = ((Int32)g_reverb.wet_gain_Q15 * (Int32)apSignal) >> 15;

    return sat16(dryPart + wetPart);
}

// -------------------------------------------------------------------------
// Processamento em bloco
// -------------------------------------------------------------------------
void processAudioReverb(Uint16* rxBlock, Uint16* txBlock, Uint16 blockSize)
{
    int i;
    for (i = 0; i < blockSize; i++) {
        txBlock[i] = (Uint16)processReverbSample((Int16)rxBlock[i]);
    }
}

// -------------------------------------------------------------------------
// Limpeza do Reverb
// -------------------------------------------------------------------------
void clearReverb(void)
{
    int i, j;

    // Limpa buffers dos combs
    for (i = 0; i < REVERB_NUM_COMBS; i++) {
        for (j = 0; j < REVERB_COMB_CHUNK; j++) {
            combBuffer[i * REVERB_COMB_CHUNK + j] = 0;
        }
        g_reverb.comb[i].ptr = 0;
    }

    // Limpa buffers dos all-pass
    for (i = 0; i < REVERB_NUM_ALLPASSES; i++) {
        for (j = 0; j < REVERB_AP_CHUNK; j++) {
            apBuffer[i * REVERB_AP_CHUNK + j] = 0;
        }
        g_reverb.allpass[i].ptr = 0;
    }
}

// -------------------------------------------------------------------------
// Controle de preset
// -------------------------------------------------------------------------
void setReverbPreset(ReverbPreset preset)
{
    if (preset >= REVERB_PRESET_COUNT) {
        preset = REVERB_PRESET_HALL;
    }
    g_reverbPreset = preset;

    // Reconfigura o reverb com os novos parâmetros
    initReverb();
}

ReverbPreset getReverbPreset(void)
{
    return g_reverbPreset;
}
