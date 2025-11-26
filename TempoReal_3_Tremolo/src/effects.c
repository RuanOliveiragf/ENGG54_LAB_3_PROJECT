//////////////////////////////////////////////////////////////////////////////
// effects.c - Implementação de efeitos de áudio para tempo real
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


// ================= TREMOLO =================
static tremolo g_tremolo;   // estado interno do tremolo


// Inicialização do tremolo (profundidade fixa, pode ajustar depois)
void initTremoloEffect(void)
{
    Int16 depth = 26214;
    tremoloInit(depth, &g_tremolo);
}


void processAudioTremolo(Uint16* rxBlock, Uint16* txBlock)
{
    int i;
    Int16 xin, yout;

    for (i = 0; i < AUDIO_BLOCK_SIZE; i++)
    {
        // 1. Converte Uint16 (formato do DMA/Codec) para Int16 (matem�tica com sinal)
        xin = (Int16)rxBlock[i];

        // 2. Processa tremolo (Agora com prote��o interna contra estouro)
        yout = tremoloProcess(xin, &g_tremolo);

        // 3. Atualiza o LFO interno para a pr�xima amostra
        tremoloSweep(&g_tremolo);

        // 4. Converte de volta para Uint16 para enviar ao buffer de sa�da
        txBlock[i] = (Uint16)yout;
    }
}
