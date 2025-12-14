//////////////////////////////////////////////////////////////////////////////
// reverb.c - Implementação do Efeito Reverb Stereo (otimizado p/ C5502)
//////////////////////////////////////////////////////////////////////////////

#include "reverb.h"

#define FS_FLOAT 48000.0f

// --- POOL DE MEMÓRIA ÚNICO ---
#pragma DATA_SECTION(g_reverbMemory, "effectsMem")
#pragma DATA_ALIGN(g_reverbMemory, 4)
static Int16 g_reverbMemory[REVERB_MEM_SIZE];

Reverb       g_reverb;

// Preset padrão
ReverbPreset g_reverbPreset = REVERB_PRESET_ROOM_2;

// --- Estrutura de Presets ---
typedef struct {
    float comb_ms[REVERB_NUM_COMBS];
    float ap_ms[REVERB_NUM_ALLPASSES];

    float comb_gains[REVERB_NUM_COMBS];
    float ap_gains[REVERB_NUM_ALLPASSES];

    float wet_gain;
    float dry_gain;

    // Damping (lowpass por shift) para os combs:
    // 0 = sem damping. 2..6 recomendado.
    Uint8 comb_damp_shift;
} ReverbPresetCfg;

// -------------------- Presets --------------------
// ROOM 2 foi ajustado para:
// - delays menores (mais "room", menos "hall")
// - feedback menor (decay mais controlado)
// - damping (menos metálico)
// - wet menor e dry um pouco reduzido (mix melhor)
static const ReverbPresetCfg REVERB_PRESETS[REVERB_PRESET_COUNT] = {

   // REV-HALL (ajustado para ficar mais parecido com o 01.wav)
    {
   // comb_ms (pequenos ajustes para quebrar ressonâncias / ficar mais natural)
       { 24.90f, 29.35f, 41.70f, 34.10f },

   // all-pass (mantém difusão, mas com menos “brilho de anel”)
       { 2.20f, 6.40f },

   // comb_gains (reduz RT/cauda e evita “sustento demais”)
       { 0.70f, 0.78f, 0.73f, 0.75f },

   // ap_gains (um pouco menor para reduzir ringing)
       { 0.60f, 0.58f },

   // wet_gain (mais próximo do seu áudio: presente, mas sem afogar)
       0.38f,
       1.00f,  // dry_gain (para manter ataque igual ao seu)
       0      // comb_damp_shift = 0 (DESLIGADO -> mantém o timbre)
     },

     // ROOM 2:
     {
         { 200.0f, 300.0f, 400.0f, 500.0f },  // comb_ms
         { 7.06f, 6.46f },                    // all-pass_ms
         { 0.50f, 0.48f, 0.56f, 0.44f },      // comb_gains
         { 0.716f, 0.613f },                  // ap_gains
         0.2f                                  // wet_gain
     },

    // REV-STAGE
     {
        { 46.27f, 39.96f, 28.03f, 51.85f },
        { 3.50f, 1.20f },
        { 0.758f, 0.854f, 0.796f, 0.825f },
        { 0.70f,  0.70f },
        0.50f,
        1.00f,
        4
    }
};

// -------------------- Helpers --------------------

static inline Int16 sat16(Int32 x) {
    if (x > 32767)  return 32767;
    if (x < -32768) return -32768;
    return (Int16)x;
}

static Int16 floatToQ15(float x) {
    if (x >= 1.0f)  return 32767;
    if (x <= -1.0f) return -32768;
    return (Int16)(x * 32768.0f);
}

static Uint16 msToSamples(float ms) {
    // Executa só na init -> não pesa (float ok)
    return (Uint16)(ms * (FS_FLOAT / 1000.0f));
}

static Int16* allocMemory(Uint16 size) {
    Int16* ptr;

    // Se faltar memória, NÃO "volta pro início" (isso estraga o reverb).
    // Em DSP limitado, melhor "clamp" e garantir estabilidade.
    if ((Uint32)g_reverb.memAllocated + size > REVERB_MEM_SIZE) {
        // Retorna NULL para o init reduzir o risco de crash
        return (Int16*)0;
    }

    ptr = &g_reverbMemory[g_reverb.memAllocated];
    g_reverb.memAllocated += size;
    return ptr;
}

// Inicializa um único núcleo de Reverb (Canal L ou R)
// use_spread: 1 para adicionar o spread (Canal R), 0 para normal (Canal L)
static void initReverbCore(ReverbCore* core, const ReverbPresetCfg* p, int use_spread)
{
    int i, j;

    // Configura Comb Filters
    for (i = 0; i < REVERB_NUM_COMBS; i++) {
        CombFilter* c = &core->comb[i];

        Uint16 samples = msToSamples(p->comb_ms[i]);
        if (use_spread) samples += REVERB_SPREAD;

        if (samples < 2) samples = 2;

        c->buffer = allocMemory(samples);
        if (!c->buffer) {
            // Falta de memória: degrade seguro -> delay mínimo
            samples = 2;
            c->buffer = &g_reverbMemory[0]; // ponteiro válido (não crash)
        }

        c->delay_samples = samples;
        c->gain_Q15      = floatToQ15(p->comb_gains[i]);
        c->ptr           = 0;

        // Damping
        c->damp_shift = p->comb_damp_shift;
        c->damp_state = 0;

        for (j = 0; j < samples; j++) c->buffer[j] = 0;
    }

    // Configura All-Pass Filters
    for (i = 0; i < REVERB_NUM_ALLPASSES; i++) {
        AllPassFilter* ap = &core->allpass[i];

        Uint16 samples = msToSamples(p->ap_ms[i]);
        if (samples < 2) samples = 2;

        ap->buffer = allocMemory(samples);
        if (!ap->buffer) {
            samples = 2;
            ap->buffer = &g_reverbMemory[0];
        }

        ap->delay_samples = samples;
        ap->gain_Q15      = floatToQ15(p->ap_gains[i]);
        ap->ptr           = 0;

        for (j = 0; j < samples; j++) ap->buffer[j] = 0;
    }
}

// Inicialização Global
void initReverb(void)
{
    g_reverb.memAllocated = 0;

    if (g_reverbPreset >= REVERB_PRESET_COUNT) g_reverbPreset = REVERB_PRESET_HALL;
    const ReverbPresetCfg* p = &REVERB_PRESETS[g_reverbPreset];

    g_reverb.wet_gain_Q15 = floatToQ15(p->wet_gain);
    g_reverb.dry_gain_Q15 = floatToQ15(p->dry_gain);

    // L sem spread
    initReverbCore(&g_reverb.left, p, 0);

    // R com spread
    initReverbCore(&g_reverb.right, p, 1);
}

// All-Pass
static Int16 processAllPass(Int16 input, AllPassFilter* apf)
{
    Int16 delayed = apf->buffer[apf->ptr];

    // v[n] = x[n] + g*d[n]
    Int32 vn = (Int32)input + (((Int32)apf->gain_Q15 * (Int32)delayed) >> 15);

    // y[n] = -g*v[n] + d[n]
    Int32 output = -(((Int32)apf->gain_Q15 * vn) >> 15) + (Int32)delayed;

    apf->buffer[apf->ptr] = sat16(vn);

    apf->ptr++;
    if (apf->ptr >= apf->delay_samples) apf->ptr = 0;

    return sat16(output);
}

// Processa uma amostra (Comb Paralelo + AP Série) com damping barato + mix dry/wet
static Int16 processReverbSample(Int16 input, ReverbCore* core, Int16 dryGain, Int16 wetGain)
{
    int i;
    Int32 accComb = 0;

    // 1) Combs em paralelo
    for (i = 0; i < REVERB_NUM_COMBS; i++) {
        CombFilter* c = &core->comb[i];

        Int16 delayed = c->buffer[c->ptr];

        // ----------------- DAMPING (barato, por shift) -----------------
        // filtered = state + (delayed - state) >> shift
        // shift=0 -> sem damping
        Int16 filtered;
        if (c->damp_shift != 0) {
            Int16 diff = (Int16)(delayed - c->damp_state);
            filtered = (Int16)(c->damp_state + (diff >> c->damp_shift));
            c->damp_state = filtered;
        } else {
            filtered = delayed;
        }
        // ---------------------------------------------------------------

        accComb += (Int32)filtered;

        // Feedback com sinal filtrado (reduz ringing)
        Int32 fb = (Int32)input + (((Int32)c->gain_Q15 * (Int32)filtered) >> 15);
        c->buffer[c->ptr] = sat16(fb);

        c->ptr++;
        if (c->ptr >= c->delay_samples) c->ptr = 0;
    }

    // Atenuação da soma dos combs
    Int16 combOut = sat16(accComb >> 2);

    // 2) All-pass em série (difusão)
    Int16 apSignal = combOut;
    for (i = 0; i < REVERB_NUM_ALLPASSES; i++) {
        apSignal = processAllPass(apSignal, &core->allpass[i]);
    }

    // 3) Mix Dry/Wet real (evita “input+wet” estourar fácil)
    Int32 dryPart = ((Int32)dryGain * (Int32)input) >> 15;
    Int32 wetPart = ((Int32)wetGain * (Int32)apSignal) >> 15;

    return sat16(dryPart + wetPart);
}

// rxBlock intercalado: L, R, L, R...
void processAudioReverb(Uint16* rxBlock, Uint16* txBlock, Uint16 blockSize)
{
    int i;
    Int16 wet = g_reverb.wet_gain_Q15;
    Int16 dry = g_reverb.dry_gain_Q15;

    for (i = 0; i < blockSize; i += 2) {

        // ESQ
        txBlock[i] = (Uint16)processReverbSample(
            (Int16)rxBlock[i],
            &g_reverb.left,
            dry,
            wet
        );

        // DIR
        txBlock[i + 1] = (Uint16)processReverbSample(
            (Int16)rxBlock[i + 1],
            &g_reverb.right,
            dry,
            wet
        );
    }
}

void clearReverb(void)
{
    initReverb();
}

void setReverbPreset(ReverbPreset preset)
{
    if (preset >= REVERB_PRESET_COUNT) preset = REVERB_PRESET_HALL;
    g_reverbPreset = preset;
    initReverb();
}

ReverbPreset getReverbPreset(void)
{
    return g_reverbPreset;
}
