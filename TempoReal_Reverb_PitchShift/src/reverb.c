/////////////////////////////////////////////////////////////////////////////
// reverb.c - Implementação Flexível (Estilo Python)
/////////////////////////////////////////////////////////////////////////////

#include "reverb.h"
#include "effects.h"
#include "dma.h"  // Necessário para AUDIO_BLOCK_SIZE
#include <math.h>
#include <stdlib.h>

// Taxa de amostragem usada para converter ms -> amostras
#define FS_FLOAT 48000.0f

// Tamanhos máximos de Buffer (para alocação de memória)
// 2400 amostras @ 48kHz = 50ms (Suficiente para Schroeder)
#define REVERB_COMB_CHUNK   2400u
#define REVERB_AP_CHUNK     480u

// Memória na SARAM (obrigatório para caber tudo)
#pragma DATA_SECTION(combBuffer, "effectsMem")
#pragma DATA_ALIGN(combBuffer, 4)
static Int16 combBuffer[REVERB_NUM_COMBS * REVERB_COMB_CHUNK];

#pragma DATA_SECTION(apBuffer, "effectsMem")
#pragma DATA_ALIGN(apBuffer, 4)
static Int16 apBuffer[REVERB_NUM_ALLPASSES * REVERB_AP_CHUNK];

Reverb g_reverb;

// ---------------------------------------------------------------------------
// Funções Auxiliares (Saturação e Conversão)
// ---------------------------------------------------------------------------
static inline Int16 sat16(Int32 x) {
    if (x > 32767)  return 32767;
    if (x < -32768) return -32768;
    return (Int16)x;
}

static Int16 floatToQ15(float x) {
    if (x >= 1.0f) return 32767;
    if (x <= -1.0f) return -32768;
    return (Int16)(x * 32768.0f);
}

// Converte milissegundos para número de amostras
static Uint16 msToSamples(float ms) {
    float samples = ms * (FS_FLOAT / 1000.0f);
    return (Uint16)samples;
}

// ---------------------------------------------------------------------------
// Inicialização Estilo Python
// Recebe arrays de configurações
// ---------------------------------------------------------------------------
void initReverb(Reverb* reverb,
                float* delays_combs_ms, float* gains_combs,
                float* delays_ap_ms,    float* gains_ap,
                float wet_level)
{
    int i, j;

    // Configura Mix Dry/Wet
    reverb->wet_gain_Q15 = floatToQ15(wet_level);
    reverb->dry_gain_Q15 = floatToQ15(1.0f - wet_level);

    // --- Configura COMBS (Paralelo) ---
    for (i = 0; i < REVERB_NUM_COMBS; i++) {
        CombFilter* c = &reverb->comb[i];

        // Ponteiro para o pedaço de memória correspondente
        c->buffer = &combBuffer[i * REVERB_COMB_CHUNK];

        // 1. Converte ms -> amostras
        Uint16 samples = msToSamples(delays_combs_ms[i]);

        // 2. Proteção de limite (Clamp)
        if (samples < 10) samples = 10;
        if (samples > REVERB_COMB_CHUNK) samples = REVERB_COMB_CHUNK;

        c->delay_samples = samples;
        c->gain_Q15      = floatToQ15(gains_combs[i]);
        c->ptr           = 0;

        // Limpa o buffer para evitar ruído inicial
        for(j=0; j < REVERB_COMB_CHUNK; j++) c->buffer[j] = 0;
    }

    // --- Configura ALL-PASS (Série) ---
    for (i = 0; i < REVERB_NUM_ALLPASSES; i++) {
        AllPassFilter* ap = &reverb->allpass[i];

        ap->buffer = &apBuffer[i * REVERB_AP_CHUNK];

        Uint16 samples = msToSamples(delays_ap_ms[i]);

        if (samples < 10) samples = 10;
        if (samples > REVERB_AP_CHUNK) samples = REVERB_AP_CHUNK;

        ap->delay_samples = samples;
        ap->gain_Q15      = floatToQ15(gains_ap[i]);
        ap->ptr           = 0;

        for(j=0; j < REVERB_AP_CHUNK; j++) ap->buffer[j] = 0;
    }
}

// ---------------------------------------------------------------------------
// Processamento All-Pass
// ---------------------------------------------------------------------------
static Int16 processAllPass(Int16 input, AllPassFilter* apf) {
    Int16 delayed = apf->buffer[apf->ptr];

    // v[n] = x[n] + g * d[n]
    Int32 vn = (Int32)input + (((Int32)apf->gain_Q15 * (Int32)delayed) >> 15);

    // y[n] = -g * v[n] + d[n]
    Int32 output = -(((Int32)apf->gain_Q15 * vn) >> 15) + (Int32)delayed;

    // Salva v[n] no buffer (com saturação pra segurança)
    apf->buffer[apf->ptr] = sat16(vn);

    apf->ptr++;
    if (apf->ptr >= apf->delay_samples) apf->ptr = 0;

    return sat16(output);
}

// ---------------------------------------------------------------------------
// Processamento Principal (Amostra por Amostra)
// ---------------------------------------------------------------------------
Int16 processReverbSample(Int16 input, Reverb* reverb)
{
    int i;
    Int32 accComb = 0;

    // Atenua entrada (divide por 4) para evitar overflow na soma dos combs
    Int16 attInput = input >> 2;

    // 1. Combs em Paralelo
    for (i = 0; i < REVERB_NUM_COMBS; i++) {
        CombFilter* c = &reverb->comb[i];

        Int16 delayed = c->buffer[c->ptr];
        accComb += (Int32)delayed;

        // Feedback: Input + Gain * Delayed
        Int32 fb = (Int32)attInput + (((Int32)c->gain_Q15 * (Int32)delayed) >> 15);
        c->buffer[c->ptr] = sat16(fb);

        c->ptr++;
        if (c->ptr >= c->delay_samples) c->ptr = 0;
    }

    // Recupera ganho da soma (Soma de 4 / 2 = Ganho x2)
    Int16 combOut = sat16(accComb >> 1);

    // 2. All-Pass em Série
    Int16 apSignal = combOut;
    for (i = 0; i < REVERB_NUM_ALLPASSES; i++) {
        apSignal = processAllPass(apSignal, &reverb->allpass[i]);
    }

    // 3. Mix Dry/Wet
    Int32 dryPart = ((Int32)reverb->dry_gain_Q15 * (Int32)input) >> 15;
    Int32 wetPart = ((Int32)reverb->wet_gain_Q15 * (Int32)apSignal) >> 15;

    return sat16(dryPart + wetPart);
}

void processAudioReverb(Uint16* rxBlock, Uint16* txBlock)
{
    int i;
    for (i = 0; i < AUDIO_BLOCK_SIZE; i++) {
        txBlock[i] = (Uint16)processReverbSample((Int16)rxBlock[i], &g_reverb);
    }
}

void clearReverbState(void) {
    // Implementação de limpeza se necessário
}
