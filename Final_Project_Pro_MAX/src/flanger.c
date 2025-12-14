//////////////////////////////////////////////////////////////////////////////
// flanger.c - Implementação Exata do Python em C
//////////////////////////////////////////////////////////////////////////////

#include "flanger.h"
#include <math.h>

#pragma DATA_SECTION(g_flangerBuffer, "effectsMem")
#pragma DATA_ALIGN(g_flangerBuffer, 4)
Int16 g_flangerBuffer[FLANGER_DELAY_SIZE];

#pragma DATA_SECTION(g_lfoTable, "effectsMem")
#pragma DATA_ALIGN(g_lfoTable, 4)
Int16 g_lfoTable[LFO_SIZE];

volatile Uint16 g_flangerWriteIndex = 0;
volatile Uint32 g_flangerPhaseAcc = 0;
volatile Uint32 g_flangerPhaseInc = 0;

static inline Int16 sat16(Int32 x) {
    if (x > 32767)  return 32767;
    if (x < -32768) return -32768;
    return (Int16)x;
}

void initFlanger(void)
{
    int i;
    float rad;
    
    // 1. Limpa Buffer
    for (i = 0; i < FLANGER_DELAY_SIZE; i++) {
        g_flangerBuffer[i] = 0;
    }
    
    // 2. Gera Tabela de Seno (Full Range -32767 a +32767)
    // Isso corresponde ao "sin(wn)" da fórmula do Python
    for (i = 0; i < LFO_SIZE; i++) {
        rad = (float)i / (float)LFO_SIZE * (2.0f * 3.14159265359f);
        g_lfoTable[i] = (Int16)(sinf(rad) * 32767.0f);
    }
    
    // 3. Configura Oscilador para 0.5 Hz
    g_flangerPhaseInc = LFO_INC;
    g_flangerPhaseAcc = 0;
    g_flangerWriteIndex = 0;
}

void processAudioFlanger(Uint16* rxBlock, Uint16* txBlock, Uint16 blockSize)
{
    int i;

    // Variáveis LFO
    Uint16 lfo_idx;
    Int32 lfo_val_Q15;

    // Variáveis Delay
    Int32 delay_Q15;
    Int16 int_delay;
    Int16 frac_delay;
    Int16 samp1, samp2, delayed_sample;
    Uint16 idx1, idx2;
    int rawIdx;

    // Áudio
    Int16 x_n;
    Int32 wet_signal, output_32;

    for (i = 0; i < blockSize; i++)
    {     
        x_n = (Int16)rxBlock[i];
        
        // --- 1. LFO (Oscilador 0.5Hz) ---
        // Incrementa fase
        g_flangerPhaseAcc += g_flangerPhaseInc;

        // Pega os 8 bits superiores para índice (2^8 = 256 = LFO_SIZE)
        lfo_idx = (Uint16)(g_flangerPhaseAcc >> 24);
        lfo_val_Q15 = (Int32)g_lfoTable[lfo_idx];

        // --- 2. CÁLCULO DO DELAY (Lógica Python: L0 + A * sin) ---
        // Delay em Q15 = (144 << 15) + (96 * Seno_Q15)
        // Isso varia o delay exatamente entre 48 e 240 amostras (1ms a 5ms)
        delay_Q15 = ((Int32)FLANGER_L0 << 15) + ((Int32)FLANGER_A * lfo_val_Q15);
        
        // Segurança
        if (delay_Q15 < 32768) delay_Q15 = 32768; // Minimo 1.0

        // Separação Inteira/Fracionária
        int_delay = (Int16)(delay_Q15 >> 15);
        frac_delay = (Int16)(delay_Q15 & 0x7FFF);

        // --- 3. LEITURA COM INTERPOLAÇÃO ---
        // Posição de leitura: (Escrita - Delay)
        rawIdx = (int)g_flangerWriteIndex - int_delay;
        while (rawIdx < 0) rawIdx += FLANGER_DELAY_SIZE;
        idx1 = (Uint16)rawIdx;

        // Amostra vizinha para interpolação
        rawIdx = (int)idx1 - 1;
        while (rawIdx < 0) rawIdx += FLANGER_DELAY_SIZE;
        idx2 = (Uint16)rawIdx;
        
        samp1 = g_flangerBuffer[idx1];
        samp2 = g_flangerBuffer[idx2];
        
        // Fórmula: y = s1 + frac * (s2 - s1)
        delayed_sample = samp1 + (Int16)(((Int32)frac_delay * (samp2 - samp1)) >> 15);
        
        // --- 4. MIXAGEM ---
        // y[n] = x[n] + gain * delayed
        // Ganho ajustado para 0.7 (22938)
        wet_signal = ((Int32)FLANGER_G * (Int32)delayed_sample) >> 15;
        output_32 = (Int32)x_n + wet_signal;
        
        txBlock[i] = (Uint16)sat16(output_32);

        // Atualiza Buffer
        g_flangerBuffer[g_flangerWriteIndex] = x_n;
        g_flangerWriteIndex++;
        if (g_flangerWriteIndex >= FLANGER_DELAY_SIZE) {
            g_flangerWriteIndex = 0;
        }
    }
}

void clearFlanger(void)
{
    initFlanger();
}
