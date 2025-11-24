/////////////////////////////////////////////////////////////////////////////
// reverb.c - Implementação do efeito de reverb (Schroeder 4 Comb + 2 AP)
/////////////////////////////////////////////////////////////////////////////

#include "reverb.h"
#include "effects.h"
#include <math.h>
#include <stdlib.h>
#include "dma.h"
// ---------------------------------------------------------------------------
// Configuração dos buffers:
// Usamos um único vetor para todos os Combs e outro para todos os All-Pass.
// Eles são particionados em "fatias" (chunks) para cada filtro.
//
// combBuffer: 4 chunks de 600 amostras = 2400 amostras (Int16)
// apBuffer  : 2 chunks de 300 amostras = 600 amostras (Int16)
//
// Esses tamanhos mantêm o uso de memória equivalente ao código original.
// ---------------------------------------------------------------------------

#define REVERB_COMB_CHUNK   600u
#define REVERB_AP_CHUNK     300u

#pragma DATA_SECTION(combBuffer, "effectsMem")
#pragma DATA_ALIGN(combBuffer, 4)
static Int16 combBuffer[REVERB_NUM_COMBS * REVERB_COMB_CHUNK];

#pragma DATA_SECTION(apBuffer, "effectsMem")
#pragma DATA_ALIGN(apBuffer, 4)
static Int16 apBuffer[REVERB_NUM_ALLPASSES * REVERB_AP_CHUNK];

// Instância global do reverb (visível em effects.c via extern em reverb.h)
Reverb g_reverb;

// ---------------------------------------------------------------------------
// Função auxiliar: saturação para 16 bits
// ---------------------------------------------------------------------------
static inline Int16 sat16(Int32 x)
{
    if (x > 32767)  return 32767;
    if (x < -32768) return -32768;
    return (Int16)x;
}

// ---------------------------------------------------------------------------
// Função auxiliar: processamento de um filtro All-Pass (EQUAÇÃO COMPLETA CORRIGIDA)
// ---------------------------------------------------------------------------
static Int16 processAllPass(Int16 input, AllPassFilter* apf)
{
    // Lê amostra atrasada do buffer (d[n] = y[n-M])
    Int16 delayed = apf->buffer[apf->ptr];
    
    // Equação completa do all-pass:
    // v[n] = x[n] + g * d[n]
    Int32 vn = (Int32)input + (((Int32)apf->gain_Q15 * (Int32)delayed) >> 15);
    vn = sat16(vn);
    
    // y[n] = -g * v[n] + d[n]
    Int32 output = -(((Int32)apf->gain_Q15 * (Int32)vn) >> 15) + (Int32)delayed;
    output = sat16(output);
    
    // Escreve v[n] no buffer (para a próxima iteração)
    apf->buffer[apf->ptr] = (Int16)vn;
    
    // Atualiza ponteiro circular
    apf->ptr++;
    if (apf->ptr >= apf->delay_samples) {
        apf->ptr = 0;
    }
    
    return (Int16)output;
}

// ---------------------------------------------------------------------------
// Inicialização dos parâmetros do reverb
// ---------------------------------------------------------------------------
void initReverb(Reverb* reverb,
                Uint16 comb_delay, float comb_gain,
                Uint16 ap_delay,   float ap_gain,
                float wet_level)
{
    int i, j;

    // --- Clamp dos parâmetros de atraso para os tamanhos de chunk ---
    if (comb_delay < 50)          comb_delay = 50;
    if (comb_delay > REVERB_COMB_CHUNK) comb_delay = REVERB_COMB_CHUNK;

    if (ap_delay < 30)            ap_delay = 30;
    if (ap_delay > REVERB_AP_CHUNK)     ap_delay = REVERB_AP_CHUNK;

    // --- Conversão de ganhos float -> Q15 ---
    if (comb_gain < 0.0f) comb_gain = 0.0f;
    if (comb_gain > 0.95f) comb_gain = 0.95f;   // evita instabilidade
    if (ap_gain < 0.0f)   ap_gain   = 0.0f;
    if (ap_gain > 0.95f)  ap_gain   = 0.95f;

    Int16 combGainQ15 = (Int16)(comb_gain * 32767.0f);
    Int16 apGainQ15   = (Int16)(ap_gain   * 32767.0f);

    // --- Wet/Dry em Q15 ---
    if (wet_level < 0.0f) wet_level = 0.0f;
    if (wet_level > 1.0f) wet_level = 1.0f;

    Int16 wetQ15 = (Int16)(wet_level * 32767.0f);
    Int16 dryQ15 = (Int16)((1.0f - wet_level) * 32767.0f);

    reverb->wet_gain_Q15 = wetQ15;
    reverb->dry_gain_Q15 = dryQ15;

    // --- Distribuição dos atrasos dos 4 Combs ---
    Uint16 combDelays[REVERB_NUM_COMBS];
    combDelays[0] = comb_delay;
    combDelays[1] = (Uint16)((comb_delay * 5) / 6);
    combDelays[2] = (Uint16)((comb_delay * 4) / 6);
    combDelays[3] = (Uint16)((comb_delay * 3) / 6);

    // Garante que nenhum atraso é 0
    for (i = 0; i < REVERB_NUM_COMBS; i++) {
        if (combDelays[i] == 0) {
            combDelays[i] = 1;
        }
    }

    // --- Distribuição dos atrasos dos 2 All-Pass ---
    Uint16 apDelays[REVERB_NUM_ALLPASSES];
    apDelays[0] = ap_delay;
    apDelays[1] = (Uint16)((ap_delay * 2) / 3);

    for (i = 0; i < REVERB_NUM_ALLPASSES; i++) {
        if (apDelays[i] == 0) {
            apDelays[i] = 1;
        }
    }

    // --- Inicializa os 4 Combs ---
    for (i = 0; i < REVERB_NUM_COMBS; i++) {
        CombFilter* c = &reverb->comb[i];

        c->buffer        = &combBuffer[i * REVERB_COMB_CHUNK];
        c->delay_samples = combDelays[i];
        c->gain_Q15      = combGainQ15;
        c->ptr           = 0;

        // Zera o trecho de buffer usado por este Comb
        for (j = 0; j < c->delay_samples; j++) {
            c->buffer[j] = 0;
        }
    }

    // --- Inicializa os 2 All-Pass ---
    for (i = 0; i < REVERB_NUM_ALLPASSES; i++) {
        AllPassFilter* apf = &reverb->allpass[i];

        apf->buffer        = &apBuffer[i * REVERB_AP_CHUNK];
        apf->delay_samples = apDelays[i];
        apf->gain_Q15      = apGainQ15;
        apf->ptr           = 0;

        // Zera o trecho de buffer usado por este All-Pass
        for (j = 0; j < apf->delay_samples; j++) {
            apf->buffer[j] = 0;
        }
    }
}

// ---------------------------------------------------------------------------
// processReverbSample (COM ALL-PASS CORRIGIDO)
// ---------------------------------------------------------------------------
Int16 processReverbSample(Int16 input, Reverb* reverb)
{
    int i;
    Int32 accComb = 0;

    // -------------------- 1) Comb filters em paralelo --------------------
    // Pré-atenuação da entrada para evitar sobrecarga (divide por 4 para 4 combs)
    Int16 attInput = (Int16)((Int32)input >> 2);

    for (i = 0; i < REVERB_NUM_COMBS; i++) {
        CombFilter* c = &reverb->comb[i];

        // Lê amostra atrasada
        Int16 delayed = c->buffer[c->ptr];

        // Acumula saída dos Combs
        accComb += (Int32)delayed;

        // Realimentação: x[n] (atenuado) + g * y[n-M]
        Int32 fb = (Int32)attInput + (((Int32)c->gain_Q15 * (Int32)delayed) >> 15);
        c->buffer[c->ptr] = sat16(fb);

        // Atualiza ponteiro circular
        c->ptr++;
        if (c->ptr >= c->delay_samples) {
            c->ptr = 0;
        }
    }

    // Média das saídas dos Combs
    Int16 combOut = sat16(accComb / REVERB_NUM_COMBS);

    // -------------------- 2) All-Pass em série (EQUAÇÃO COMPLETA) -----------------
    Int16 apSignal = combOut;
    
    // Processa os dois all-pass em série usando a função corrigida
    for (i = 0; i < REVERB_NUM_ALLPASSES; i++) {
        apSignal = processAllPass(apSignal, &reverb->allpass[i]);
    }

    // -------------------- 3) Mix Dry/Wet -------------------------------
    Int32 dry32 = ((Int32)reverb->dry_gain_Q15 * (Int32)input) >> 15;
    Int32 wet32 = ((Int32)reverb->wet_gain_Q15 * (Int32)apSignal) >> 15;

    Int32 out32 = dry32 + wet32;
    Int16 out   = sat16(out32);

    if (out > 32767.0f) {
        out = 32767;
    }
    else if (out < -32768.0f) {
        out = -32768;
    }
    else {
        out = (Int16)out;
    }

    return out;
}

// ---------------------------------------------------------------------------
// processAudioReverb
// ---------------------------------------------------------------------------
void processAudioReverb(Uint16* rxBlock, Uint16* txBlock)
{
    int   i;
    Int16 xin, yout;

    for (i = 0; i < AUDIO_BLOCK_SIZE; i++) {
        // Converte Uint16 (dados do codec) para Int16
        xin = (Int16)rxBlock[i];

        // Processa uma amostra no reverb
        yout = processReverbSample(xin, &g_reverb);

        // Converte de volta para Uint16
        txBlock[i] = (Uint16)yout;
    }
}

void clearReverbState(void) {
    int i, k;

    // Zera buffers dos Combs
    for(i=0; i < REVERB_NUM_COMBS; i++) {
        CombFilter* c = &g_reverb.comb[i];
        c->ptr = 0;
        for(k=0; k < c->delay_samples; k++) {
            c->buffer[k] = 0;
        }
    }
    // Zera buffers dos All-Pass
    for(i=0; i < REVERB_NUM_ALLPASSES; i++) {
        AllPassFilter* ap = &g_reverb.allpass[i];
        ap->ptr = 0;
        for(k=0; k < ap->delay_samples; k++) {
            ap->buffer[k] = 0;
        }
    }
}

