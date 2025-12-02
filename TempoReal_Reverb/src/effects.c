#include "effects.h"
#include <math.h>
#include "dma.h"
#include "reverb.h"

void initReverbEffect(void)
{
    // 1. Defina os DELAYS em MILISSEGUNDOS (ms)
    // Valores clássicos do Schroeder Reverb para som natural
    static float comb_ms[4]   = {29.7f, 37.1f, 41.1f, 43.7f};
    static float ap_ms[2]     = {5.0f,  1.7f};

    // 2. Defina os GANHOS (0.0 a 0.99)
    // Combs: feedback alto para cauda longa (0.8 a 0.9)
    // APs: geralmente 0.7 para difusÃ£o
    static float comb_gains[4] = {0.84f, 0.82f, 0.80f, 0.78f};
    static float ap_gains[2]   = {0.70f, 0.70f};

    // 3. Mix (Wet level)
    float wet = 0.8f;

    // Inicializa passando os arrays
    initReverb(&g_reverb,
               comb_ms, comb_gains,
               ap_ms,   ap_gains,
               wet);
}

void processAudioReverbWrapper(Uint16* rxBlock, Uint16* txBlock)
{
    processAudioReverb(rxBlock, txBlock);
}
