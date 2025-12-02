#include "effects.h"
#include <math.h>
#include "dma.h"
#include "reverb.h"
#include "pitch_shift.h"

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
    // --- SOLUÇÃO ---
    // Em vez de processar amostra por amostra (lento e usa a função dummy),
    // chamamos a função otimizada que processa o bloco inteiro de uma vez.

    // 1. Apenas Pitch Shift (Sem Reverb por enquanto):
    //processAudioPitchShift(rxBlock, txBlock);

     // SE QUISER LIGAR O REVERB DEPOIS (CADEIA):
    // O Pitch Shift precisa processar o resultado do Reverb.
    // Como processAudioPitchShift lê de um array e escreve em outro, podemos fazer assim:


    int i;
    // 1. Processa Reverb: Entrada(rx) -> Saída Temporária(tx)
    for (i = 0; i < AUDIO_BLOCK_SIZE; i++) {
        txBlock[i] = (Uint16)processReverbSample((Int16)rxBlock[i], &g_reverb);
    }

    // 2. Processa Pitch: Entrada(tx) -> Saída(tx) (In-place)
    // Passamos txBlock como entrada E saída. A função suporta isso.
    processAudioPitchShift(txBlock, txBlock);

}
