//////////////////////////////////////////////////////////////////////////////
// effects.c - Implementação de efeitos de áudio (Adaptado)
//////////////////////////////////////////////////////////////////////////////

#include "effects.h"
#include <math.h>
#include "dma.h"

// =================== DEFINIÇÕES E CONSTANTES ===================
#define PI 3.1415926f
#define FS 48000.0f

// Constantes do Tremolo (frequência do LFO)
#define FR 3.0f
#define TREMOLO_DEPTH 0.8f
#define W (2.0f * PI * FR / FS)

// Constantes do Flanger (caso não estejam em effects.h)
#ifndef FLANGER_DELAY_SIZE
#define FLANGER_DELAY_SIZE 1024 // Exemplo de tamanho seguro
#endif
#ifndef LFO_SIZE
#define LFO_SIZE 1024
#endif


// =================== VARIÁVEIS GLOBAIS (ESTADO) ===================
// Adaptadas para os nomes que suas novas funções exigem (g_...)

// Estado do Flanger
Int16 g_flangerBuffer[FLANGER_DELAY_SIZE];
Uint16 g_flangerWriteIndex = 0;
Int16 g_lfoTable[LFO_SIZE];
Uint16 g_lfoIndex = 0;

// Estado do Tremolo
tremolo_t g_tremolo;

// Controle Geral
static Uint8 currentActiveEffect = EFFECT_LOOPBACK;

// =================== INICIALIZAÇÃO DINÂMICA ===================

void initEffect(Uint8 effect) {
    int i;
    float rad;
    
    // Limpa qualquer estado anterior
    cleanupEffect(currentActiveEffect);
    
    switch(effect) {
        case EFFECT_LOOPBACK:
            break;

        case EFFECT_FLANGER:
            // 1. Zera o buffer de áudio circular
            for(i = 0; i < FLANGER_DELAY_SIZE; i++) {
                g_flangerBuffer[i] = 0;
            }
            
            // 2. Gera a tabela LFO em formato Q15 (necessário para sua nova função)
            // A tabela vai de -32767 a +32767
            for(i = 0; i < LFO_SIZE; i++) {
                rad = (float)i / LFO_SIZE * (2.0f * PI);
                g_lfoTable[i] = (Int16)(sinf(rad) * 32767.0f);
            }
            
            g_flangerWriteIndex = 0;
            g_lfoIndex = 0;
            break;
            
        case EFFECT_TREMOLO:
            // Inicializa estado do tremolo
            g_tremolo.mod = 0.0f;
            g_tremolo.n = 0;
            break;
    }
    
    currentActiveEffect = effect;
}

void cleanupEffect(Uint8 oldEffect) {
    // Em sistemas embarcados sem alocação dinâmica (malloc),
    // geralmente não é necessário "free", apenas resetar estados se preciso.
    if (oldEffect == EFFECT_TREMOLO) {
        g_tremolo.n = 0;
    }
}

// =================== FUNÇÕES AUXILIARES (TREMOLO) ===================

// Implementação da varredura do Tremolo (Fornecida por você)
void tremoloSweep(tremolo_t *t)
{
    t->n++;
    // Nota: Certifique-se que 'n' não cresça indefinidamente ou que sin aguente valores grandes
    if (t->n >= FS) t->n = 0; // Reset de segurança opcional para manter precisão do float
    t->mod = sinf(W * t->n);
}

// Implementação do Processamento do Tremolo (Adicionada para compilar)
// Lógica: Gain = (1 - Depth) + Depth * Mod_Normalizado
Int16 tremoloProcess(Int16 xin, tremolo_t *t) {
    float modNorm, gain;

    // Normaliza o mod de [-1, 1] para [0, 1]
    modNorm = 0.5f * (t->mod + 1.0f);

    // Calcula ganho atual
    gain = (1.0f - TREMOLO_DEPTH) + (TREMOLO_DEPTH * modNorm);

    // Aplica ganho e retorna
    return (Int16)(xin * gain);
}

// =================== PROCESSAMENTO DE EFEITOS ===================

void processAudioLoopback(Uint16* rxBlock, Uint16* txBlock) {
    int i;
    for(i = 0; i < AUDIO_BLOCK_SIZE; i++) {
        txBlock[i] = rxBlock[i];
    }
}

// ---------------------------------------------------------
// NOVA FUNÇÃO: FLANGER (Inserida conforme solicitado)
// ---------------------------------------------------------
void processAudioFlanger(Uint16* rxBlock, Uint16* txBlock)
{
    int i;
    Int32 lfoSin_Q15;     // Valor do LFO
    Int32 currentDelay_L; // Atraso atual em amostras
    Int16 y_n, x_n, x_n_L;  // y(n), x(n), e x[n-L(n)]
    Uint16 readIndex;
    Int32 temp_out;       // Variável de 32 bits para calcular a soma com segurança

    for (i = 0; i < AUDIO_BLOCK_SIZE; i++)
    {
        // 1. Lê a amostra de entrada
        x_n = (Int16)rxBlock[i];

        // 2. Obtém valor do LFO (M(n))
        lfoSin_Q15 = (Int32)g_lfoTable[g_lfoIndex];

        // Avança o ponteiro do LFO (circular)
        g_lfoIndex = (g_lfoIndex + 1) % LFO_SIZE;

        // 3. Calcula L(n) = L0 + (A * M(n))
        // FLANGER_A está em Q15, shift >> 16 ajusta a escala
        // Nota: Certifique-se que FLANGER_L0 e FLANGER_A estão definidos no effects.h
        currentDelay_L = FLANGER_L0 + ((FLANGER_A * lfoSin_Q15) >> 16);

        // Proteção: Garante que o atraso está dentro dos limites do buffer
        if (currentDelay_L < 1) currentDelay_L = 1;
        if (currentDelay_L >= FLANGER_DELAY_SIZE) currentDelay_L = FLANGER_DELAY_SIZE - 1;

        // 4. Encontra o índice de leitura no buffer circular
        readIndex = (g_flangerWriteIndex - (int)currentDelay_L + FLANGER_DELAY_SIZE) % FLANGER_DELAY_SIZE;

        // x_n_L é a amostra atrasada x[n - L(n)]
        x_n_L = g_flangerBuffer[readIndex];

        // 5. Cálculo do Efeito com Proteção de Saturação
        // Primeiro calculamos a parte do efeito (Wet)
        // Nota: Certifique-se que FLANGER_g está definido no effects.h
        Int32 wet_part = ((Int32)FLANGER_g * (Int32)x_n_L) >> 15;

        // Somamos em 32 bits
        temp_out = (Int32)x_n + wet_part;

        // Lógica de Saturação
        if (temp_out > 32767) {
            y_n = 32767;
        } else if (temp_out < -32768) {
            y_n = -32768;
        } else {
            y_n = (Int16)temp_out;
        }

        // 6. Escreve a saída
        txBlock[i] = (Uint16)y_n;

        // 7. Atualiza buffer circular
        g_flangerBuffer[g_flangerWriteIndex] = x_n;
        g_flangerWriteIndex = (g_flangerWriteIndex + 1) % FLANGER_DELAY_SIZE;
    }
}

// ---------------------------------------------------------
// NOVA FUNÇÃO: TREMOLO (Inserida conforme solicitado)
// ---------------------------------------------------------
void processAudioTremolo(Uint16* rxBlock, Uint16* txBlock)
{
    int i;
    Int16 xin, yout;

    for (i = 0; i < AUDIO_BLOCK_SIZE; i++)
    {
        // 1. Converte Uint16 para Int16
        xin = (Int16)rxBlock[i];

        // 2. Processa tremolo
        yout = tremoloProcess(xin, &g_tremolo);

        // 3. Atualiza o LFO interno
        tremoloSweep(&g_tremolo);

        // 4. Converte de volta para Uint16
        txBlock[i] = (Uint16)yout;
    }
}
