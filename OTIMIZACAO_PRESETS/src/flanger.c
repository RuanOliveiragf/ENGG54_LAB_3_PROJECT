//////////////////////////////////////////////////////////////////////////////
// flanger.c - Implementação do Efeito Flanger
//////////////////////////////////////////////////////////////////////////////

#include "flanger.h"
#include <math.h>

// Buffers alocados na seção especial para efeitos
#pragma DATA_SECTION(g_flangerBuffer, "effectsMem")
#pragma DATA_ALIGN(g_flangerBuffer, 4)
Int16 g_flangerBuffer[FLANGER_DELAY_SIZE];

#pragma DATA_SECTION(g_lfoTable, "effectsMem")
Int16 g_lfoTable[LFO_SIZE];

volatile Uint16 g_flangerWriteIndex = 0;
volatile Uint16 g_lfoIndex = 0;

// Inicialização do Flanger
void initFlanger(void)
{
    int i;
    float rad;
    
    // Limpa buffer do flanger
    for (i = 0; i < FLANGER_DELAY_SIZE; i++) {
        g_flangerBuffer[i] = 0;
    }
    
    // Gera tabela LFO (seno em Q15)
    for (i = 0; i < LFO_SIZE; i++) {
        rad = (float)i / LFO_SIZE * (2.0f * 3.14159265359f);
        g_lfoTable[i] = (Int16)(sinf(rad) * 32767.0f);
    }
    
    // Reseta índices
    g_flangerWriteIndex = 0;
    g_lfoIndex = 0;
}

// Processamento do Flanger
void processAudioFlanger(Uint16* rxBlock, Uint16* txBlock, Uint16 blockSize)
{
    int i;
    Int32 lfoSin_Q15;
    Int32 currentDelay_L;
    Int16 y_n, x_n, x_n_L;
    Uint16 readIndex;

    for (i = 0; i < blockSize; i++)
    {     
        // Amostra de entrada
        x_n = rxBlock[i];
        
        // Obtém valor do LFO
        lfoSin_Q15 = (Int32)g_lfoTable[g_lfoIndex];
        g_lfoIndex = (g_lfoIndex + 1) % LFO_SIZE;
        
        // Calcula delay variável L(n) = L0 + (A * M(n))
        currentDelay_L = FLANGER_L0 + ((FLANGER_A * lfoSin_Q15) >> 16);
        
        // Limita delay
        if (currentDelay_L < 1) currentDelay_L = 1;
        if (currentDelay_L >= FLANGER_DELAY_SIZE) currentDelay_L = FLANGER_DELAY_SIZE - 1;
        
        // Calcula índice de leitura no buffer circular
        readIndex = (g_flangerWriteIndex - (int)currentDelay_L + FLANGER_DELAY_SIZE) % FLANGER_DELAY_SIZE;
        x_n_L = g_flangerBuffer[readIndex];
        
        // Aplica equação do flanger: y(n) = x(n) + g * x[n - L(n)]
        y_n = x_n + (Int16)(((Int32)FLANGER_g * (Int32)x_n_L) >> 15);
        txBlock[i] = y_n;
        
        // Atualiza buffer circular
        g_flangerBuffer[g_flangerWriteIndex] = x_n;
        g_flangerWriteIndex = (g_flangerWriteIndex + 1) % FLANGER_DELAY_SIZE;
    }
}

// Limpeza do Flanger
void clearFlanger(void)
{
    int i;
    
    // Limpa buffer
    for (i = 0; i < FLANGER_DELAY_SIZE; i++) {
        g_flangerBuffer[i] = 0;
    }
    
    // Reseta índices
    g_flangerWriteIndex = 0;
    g_lfoIndex = 0;
}