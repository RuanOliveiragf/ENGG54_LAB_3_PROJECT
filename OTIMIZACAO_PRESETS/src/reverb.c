//////////////////////////////////////////////////////////////////////////////
// reverb.c - Implementação do Efeito Reverb Stereo
//////////////////////////////////////////////////////////////////////////////

#include "reverb.h"
#include <math.h>

#define FS_FLOAT 48000.0f

// --- POOL DE MEMÓRIA ÚNICO ---
#pragma DATA_SECTION(g_reverbMemory, "effectsMem")
#pragma DATA_ALIGN(g_reverbMemory, 4)
static Int16 g_reverbMemory[REVERB_MEM_SIZE];

Reverb       g_reverb;
ReverbPreset g_reverbPreset = REVERB_PRESET_HALL;

// --- Estrutura de Presets (Mantida igual ao original) ---
typedef struct {
    float comb_ms[REVERB_NUM_COMBS];
    float ap_ms[REVERB_NUM_ALLPASSES];
    float comb_gains[REVERB_NUM_COMBS];
    float ap_gains[REVERB_NUM_ALLPASSES];
    float wet_gain;
} ReverbPresetCfg;

static const ReverbPresetCfg REVERB_PRESETS[REVERB_PRESET_COUNT] = {
    // REV-HALL
    { { 26.84f, 28.02f, 45.82f, 33.63f }, { 2.35f, 6.90f }, { 0.758f, 0.854f, 0.796f, 0.825f }, { 0.653f, 0.659f }, 0.4f },
    // REV-ROOM
    { { 419.0f, 359.0f, 251.0f, 467.0f }, { 7.06f, 6.46f }, { 0.50f, 0.48f, 0.56f, 0.44f },   { 0.716f, 0.613f }, 0.2f },
    // REV-STAGE
    { { 46.27f, 39.96f, 28.03f, 51.85f }, { 3.50f, 1.20f }, { 0.758f, 0.854f, 0.796f, 0.825f }, { 0.70f,  0.70f },  0.5f }
};

// Funções Auxiliares
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
    return (Uint16)(ms * (FS_FLOAT / 1000.0f));
}

static Int16* allocMemory(Uint16 size) {
    Int16* ptr;
    // Verifica overflow de memória
    if ((Uint32)g_reverb.memAllocated + size > REVERB_MEM_SIZE) {
        // Se faltar memória, aponta para o início (erro, mas evita crash)
        // Idealmente, ajuste REVERB_MEM_SIZE
        return &g_reverbMemory[0];
    }
    ptr = &g_reverbMemory[g_reverb.memAllocated];
    g_reverb.memAllocated += size;
    return ptr;
}

// Inicializa um único núcleo de Reverb (Canal L ou R)
// offset_buffer: índice onde começa a memória deste canal
// use_spread: 1 para adicionar o spread (Canal R), 0 para normal (Canal L)
static void initReverbCore(ReverbCore* core, const ReverbPresetCfg* p, int use_spread)
{
    int i, j;

    // Configura Comb Filters
    for (i = 0; i < REVERB_NUM_COMBS; i++) {
        CombFilter* c = &core->comb[i];

        // Calcula tamanho necessário
        Uint16 samples = msToSamples(p->comb_ms[i]);
        if (use_spread) samples += REVERB_SPREAD;

        // Segurança mínima
        if (samples < 2) samples = 2;

        // Aloca EXATAMENTE o que precisa
        c->buffer = allocMemory(samples);
        c->delay_samples = samples;
        c->gain_Q15      = floatToQ15(p->comb_gains[i]);
        c->ptr           = 0;

        // Limpa buffer
        for(j=0; j<samples; j++) c->buffer[j] = 0;
    }

    // Configura All-Pass Filters
    for (i = 0; i < REVERB_NUM_ALLPASSES; i++) {
        AllPassFilter* ap = &core->allpass[i];

        Uint16 samples = msToSamples(p->ap_ms[i]);
        if (samples < 2) samples = 2;

        ap->buffer = allocMemory(samples);
        ap->delay_samples = samples;
        ap->gain_Q15      = floatToQ15(p->ap_gains[i]);
        ap->ptr           = 0;

        for(j=0; j<samples; j++) ap->buffer[j] = 0;
    }
}

// Inicialização Global
void initReverb(void)
{
    // Reseta alocador de memória
    g_reverb.memAllocated = 0;

    if (g_reverbPreset >= REVERB_PRESET_COUNT) g_reverbPreset = REVERB_PRESET_HALL;
    const ReverbPresetCfg* p = &REVERB_PRESETS[g_reverbPreset];

    g_reverb.wet_gain_Q15 = floatToQ15(p->wet_gain);

    // Inicializa L (sem spread)
    initReverbCore(&g_reverb.left, p, 0);

    // Inicializa R (com spread de 23 amostras)
    initReverbCore(&g_reverb.right, p, 1);
}

// Processa All-Pass (Genérico)
static Int16 processAllPass(Int16 input, AllPassFilter* apf)
{
    Int16 delayed = apf->buffer[apf->ptr];

    // v[n] = x[n] + g * d[n]
    Int32 vn = (Int32)input + (((Int32)apf->gain_Q15 * (Int32)delayed) >> 15);

    // y[n] = -g * v[n] + d[n]
    Int32 output = -(((Int32)apf->gain_Q15 * vn) >> 15) + (Int32)delayed;

    apf->buffer[apf->ptr] = sat16(vn);

    apf->ptr++;
    if (apf->ptr >= apf->delay_samples) apf->ptr = 0;

    return sat16(output);
}

// Processa uma amostra completa (Comb Paralelo + AP Série)
static Int16 processReverbSample(Int16 input, ReverbCore* core, Int16 wetGain)
{
    int i;
    Int32 accComb = 0;

    // 1. Combs em paralelo
    for (i = 0; i < REVERB_NUM_COMBS; i++) {
        CombFilter* c = &core->comb[i];
        Int16 delayed = c->buffer[c->ptr];

        accComb += (Int32)delayed;

        // Feedback
        Int32 fb = (Int32)input + (((Int32)c->gain_Q15 * (Int32)delayed) >> 15);
        c->buffer[c->ptr] = sat16(fb);

        c->ptr++;
        if (c->ptr >= c->delay_samples) c->ptr = 0;
    }

    // Atenuação da soma dos combs (senão satura rápido)
    Int16 combOut = sat16(accComb >> 2);

    // 2. All-pass em série
    Int16 apSignal = combOut;
    for (i = 0; i < REVERB_NUM_ALLPASSES; i++) {
        apSignal = processAllPass(apSignal, &core->allpass[i]);
    }

    // 3. Mix Wet
    Int32 wetPart = ((Int32)wetGain * (Int32)apSignal) >> 15;

    // Soma com Dry (Input)
    return sat16((Int32)input + wetPart);
}

// --- LOOP PRINCIPAL (STEREO) ---
// O buffer rxBlock vem intercalado: L, R, L, R...
void processAudioReverb(Uint16* rxBlock, Uint16* txBlock, Uint16 blockSize)
{
    int i;
    Int16 wet = g_reverb.wet_gain_Q15;

    // blockSize é o total de elementos (amostras L + R)
    // Avançamos de 2 em 2 para processar o par estéreo
    for (i = 0; i < blockSize; i += 2) {

        // --- CANAL ESQUERDO (i) ---
        // Usa g_reverb.left (delays originais)
        txBlock[i] = (Uint16)processReverbSample(
            (Int16)rxBlock[i],
            &g_reverb.left,
            wet
        );

        // --- CANAL DIREITO (i+1) ---
        // Usa g_reverb.right (delays + spread)
        txBlock[i+1] = (Uint16)processReverbSample(
            (Int16)rxBlock[i+1],
            &g_reverb.right,
            wet
        );
    }
}

void clearReverb(void)
{
    // Apenas reinicializa, o que zera os ponteiros e limpa buffers
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
