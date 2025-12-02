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

// Variável global do reverb
Reverb g_reverb;

// Funções auxiliares
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

static Uint16 msToSamples(float ms) {
    float samples = ms * (FS_FLOAT / 1000.0f);
    return (Uint16)samples;
}

// Inicialização do Reverb
void initReverb(void)
{
    int i, j;
    
    // Valores otimizados para DSP (memória reduzida)
    static float comb_ms[4]   = {20.0f, 25.0f, 30.0f, 35.0f};
    static float ap_ms[2]     = {3.0f,  1.5f};
    static float comb_gains[4] = {0.70f, 0.68f, 0.66f, 0.64f};
    static float ap_gains[2]   = {0.65f, 0.65f};
    
    // Configura Mix Dry/Wet (70% wet)
    g_reverb.wet_gain_Q15 = floatToQ15(0.7f);
    g_reverb.dry_gain_Q15 = floatToQ15(0.3f);
    
    // Configura COMBS (Paralelo)
    for (i = 0; i < REVERB_NUM_COMBS; i++) {
        CombFilter* c = &g_reverb.comb[i];
        c->buffer = &combBuffer[i * REVERB_COMB_CHUNK];
        
        Uint16 samples = msToSamples(comb_ms[i]);
        if (samples < 10) samples = 10;
        if (samples > REVERB_COMB_CHUNK) samples = REVERB_COMB_CHUNK;
        
        c->delay_samples = samples;
        c->gain_Q15 = floatToQ15(comb_gains[i]);
        c->ptr = 0;
        
        for (j = 0; j < REVERB_COMB_CHUNK; j++) {
            c->buffer[j] = 0;
        }
    }
    
    // Configura ALL-PASS (Série)
    for (i = 0; i < REVERB_NUM_ALLPASSES; i++) {
        AllPassFilter* ap = &g_reverb.allpass[i];
        ap->buffer = &apBuffer[i * REVERB_AP_CHUNK];
        
        Uint16 samples = msToSamples(ap_ms[i]);
        if (samples < 10) samples = 10;
        if (samples > REVERB_AP_CHUNK) samples = REVERB_AP_CHUNK;
        
        ap->delay_samples = samples;
        ap->gain_Q15 = floatToQ15(ap_gains[i]);
        ap->ptr = 0;
        
        for (j = 0; j < REVERB_AP_CHUNK; j++) {
            ap->buffer[j] = 0;
        }
    }
}

// Processamento All-Pass
static Int16 processAllPass(Int16 input, AllPassFilter* apf) {
    Int16 delayed = apf->buffer[apf->ptr];
    
    // v[n] = x[n] + g * d[n]
    Int32 vn = (Int32)input + (((Int32)apf->gain_Q15 * (Int32)delayed) >> 15);
    
    // y[n] = -g * v[n] + d[n]
    Int32 output = -(((Int32)apf->gain_Q15 * vn) >> 15) + (Int32)delayed;
    
    // Salva v[n] no buffer
    apf->buffer[apf->ptr] = sat16(vn);
    
    apf->ptr++;
    if (apf->ptr >= apf->delay_samples) apf->ptr = 0;
    
    return sat16(output);
}

// Processamento do Reverb (amostra por amostra)
static Int16 processReverbSample(Int16 input)
{
    int i;
    Int32 accComb = 0;
    
    // Atenua entrada para evitar overflow
    Int16 attInput = input >> 2;
    
    // Combs em Paralelo
    for (i = 0; i < REVERB_NUM_COMBS; i++) {
        CombFilter* c = &g_reverb.comb[i];
        Int16 delayed = c->buffer[c->ptr];
        accComb += (Int32)delayed;
        
        // Feedback: Input + Gain * Delayed
        Int32 fb = (Int32)attInput + (((Int32)c->gain_Q15 * (Int32)delayed) >> 15);
        c->buffer[c->ptr] = sat16(fb);
        
        c->ptr++;
        if (c->ptr >= c->delay_samples) c->ptr = 0;
    }
    
    // Soma dos combs
    Int16 combOut = sat16(accComb >> 1);
    
    // All-Pass em Série
    Int16 apSignal = combOut;
    for (i = 0; i < REVERB_NUM_ALLPASSES; i++) {
        apSignal = processAllPass(apSignal, &g_reverb.allpass[i]);
    }
    
    // Mix Dry/Wet
    Int32 dryPart = ((Int32)g_reverb.dry_gain_Q15 * (Int32)input) >> 15;
    Int32 wetPart = ((Int32)g_reverb.wet_gain_Q15 * (Int32)apSignal) >> 15;
    
    return sat16(dryPart + wetPart);
}

// Processamento em bloco
void processAudioReverb(Uint16* rxBlock, Uint16* txBlock, Uint16 blockSize)
{
    int i;
    for (i = 0; i < blockSize; i++) {
        txBlock[i] = (Uint16)processReverbSample((Int16)rxBlock[i]);
    }
}

// Limpeza do Reverb
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