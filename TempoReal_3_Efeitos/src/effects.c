//////////////////////////////////////////////////////////////////////////////
// effects.c - ImplementaÃ§Ã£o de efeitos de Ã¡udio para tempo real
//////////////////////////////////////////////////////////////////////////////

#include "effects.h"
#include <math.h>
#include "dma.h"
#include "tremolo.h"   // <- tremolo do livro
#include "reverb.h"    // <- novo: reverb

// Buffer circular para flanger
#pragma DATA_SECTION(g_flangerBuffer, "effectsMem")
#pragma DATA_ALIGN(g_flangerBuffer, 4)
Int16 g_flangerBuffer[FLANGER_DELAY_SIZE];

volatile Uint16 g_flangerWriteIndex = 0;

// Tabela LFO para flanger
Int16 g_lfoTable[LFO_SIZE];
volatile Uint16 g_lfoIndex = 0;
float g_lfoPhaseInc = (LFO_SIZE * FLANGER_fr) / 48000.0f; // Para FS=48kHz

// ================= TREMOLO =================
static tremolo g_tremolo;   // estado interno do tremolo

// ================= REVERB =================
// (jÃ¡ inicializado em reverb.c)

void initFlanger(void)
{
    int i;
    float rad;

    // Inicializa buffer do flanger com zeros
    for (i = 0; i < FLANGER_DELAY_SIZE; i++) {
        g_flangerBuffer[i] = 0;
    }

    // Gera tabela LFO (seno em Q15)
    for (i = 0; i < LFO_SIZE; i++) {
        rad = (float)i / LFO_SIZE * (2.0f * 3.14159265359f);
        g_lfoTable[i] = (Int16)(sinf(rad) * 32767.0f);
    }

    g_flangerWriteIndex = 0;
    g_lfoIndex = 0;
}

// InicializaÃ§Ã£o do tremolo (profundidade fixa, pode ajustar depois)
void initTremoloEffect(void)
{
    float depth = 0.8f;  // 0 a 1 (0.8 = tremolo forte)
    tremoloInit(depth, &g_tremolo);
}

// InicializaÃ§Ã£o do reverb (COM PARÃ‚METROS MAIS CONSERVADORES)
void initReverbEffect(void)
{
    // ParÃ¢metros ajustados para maior estabilidade
    initReverb(&g_reverb, 
               2064,    // ~43ms a 48kHz (2064 amostras)
               0.5f,    // ganho comb REDUZIDO para maior estabilidade
               466,     // ~9.7ms a 48kHz (466 amostras) 
               0.7f,    // ganho all-pass
               0.15f);  // wet gain (15% - ajustÃ¡vel conforme necessidade)
}

void processAudioLoopback(Uint16* rxBlock, Uint16* txBlock)
{
    int i;
    for (i = 0; i < AUDIO_BLOCK_SIZE; i++) {
        txBlock[i] = rxBlock[i];
    }
}

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
        // 1. Lê a amostra de entrada (Convertendo Uint16 do DMA para Int16 com sinal)
        x_n = (Int16)rxBlock[i];

        // 2. Obtém valor do LFO (M(n))
        lfoSin_Q15 = (Int32)g_lfoTable[g_lfoIndex];

        // Avança o ponteiro do LFO (circular)
        g_lfoIndex = (g_lfoIndex + 1) % LFO_SIZE;

        // 3. Calcula L(n) = L0 + (A * M(n))
        // FLANGER_A está em Q15, shift >> 16 ajusta a escala
        currentDelay_L = FLANGER_L0 + ((FLANGER_A * lfoSin_Q15) >> 16);

        // Proteção: Garante que o atraso está dentro dos limites do buffer
        if (currentDelay_L < 1) currentDelay_L = 1;
        if (currentDelay_L >= FLANGER_DELAY_SIZE) currentDelay_L = FLANGER_DELAY_SIZE - 1;

        // 4. Encontra o índice de leitura no buffer circular
        // (WritePointer - Delay + BufferSize) % BufferSize
        readIndex = (g_flangerWriteIndex - (int)currentDelay_L + FLANGER_DELAY_SIZE) % FLANGER_DELAY_SIZE;

        // x_n_L é a amostra atrasada x[n - L(n)]
        x_n_L = g_flangerBuffer[readIndex];

        // 5. Cálculo do Efeito com Proteção de Saturação
        // Primeiro calculamos a parte do efeito (Wet)
        Int32 wet_part = ((Int32)FLANGER_g * (Int32)x_n_L) >> 15;

        // Somamos em 32 bits (Int32) para ter espaço extra ("headroom") temporário
        temp_out = (Int32)x_n + wet_part;

        // Lógica de Saturação:
        // Se a soma for maior que o máximo que 16 bits aguenta (32767), trava no máximo.
        // Se for menor que o mínimo (-32768), trava no mínimo.
        if (temp_out > 32767) {
            y_n = 32767;
        } else if (temp_out < -32768) {
            y_n = -32768;
        } else {
            y_n = (Int16)temp_out;
        }

        // 6. Escreve a saída (convertendo de volta para Uint16)
        txBlock[i] = (Uint16)y_n;

        // 7. Atualiza buffer circular com a amostra atual para uso futuro
        g_flangerBuffer[g_flangerWriteIndex] = x_n;
        g_flangerWriteIndex = (g_flangerWriteIndex + 1) % FLANGER_DELAY_SIZE;
    }
}

void clearFlangerState(void) {
    int i;
    // Reseta o índice de escrita
    g_flangerWriteIndex = 0;

    // Zera toda a memória do delay
    for(i=0; i < FLANGER_DELAY_SIZE; i++) {
        g_flangerBuffer[i] = 0;
    }
}

void processAudioTremolo(Uint16* rxBlock, Uint16* txBlock)
{
    int i;
    Int16 xin, yout;

    for (i = 0; i < AUDIO_BLOCK_SIZE; i++)
    {
        // 1. Converte Uint16 (formato do DMA/Codec) para Int16 (matemática com sinal)
        xin = (Int16)rxBlock[i];

        // 2. Processa tremolo (Agora com proteção interna contra estouro)
        yout = tremoloProcess(xin, &g_tremolo);

        // 3. Atualiza o LFO interno para a próxima amostra
        tremoloSweep(&g_tremolo);

        // 4. Converte de volta para Uint16 para enviar ao buffer de saída
        txBlock[i] = (Uint16)yout;
    }
}

void resetTremoloState(void) {
    // Reseta o índice do LFO para o início (fase 0)
    g_tremolo.n = 0;
}
