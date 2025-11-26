/////////////////////////////////////////////////////////////////////////////
// reverb.h - Header para efeito de reverb (Schroeder: 4 Comb + 2 All-Pass)
/////////////////////////////////////////////////////////////////////////////

#ifndef REVERB_H_
#define REVERB_H_

#include "tistdtypes.h"

// Quantidade de filtros na estrutura de Schroeder
#define REVERB_NUM_COMBS       4
#define REVERB_NUM_ALLPASSES   2

// Estrutura para filtro Comb (feedback)
typedef struct {
    Int16* buffer;        // Ponteiro para o buffer circular
    Uint16 delay_samples; // Tamanho efetivo do atraso (em amostras)
    Int16  gain_Q15;      // Ganho de realimentação em Q15
    Uint16 ptr;           // �ndice de escrita/leitura (ponteiro circular)
} CombFilter;

// Estrutura para filtro All-Pass
typedef struct {
    Int16* buffer;        // Ponteiro para o buffer circular
    Uint16 delay_samples; // Tamanho efetivo do atraso (em amostras)
    Int16  gain_Q15;      // Ganho em Q15
    Uint16 ptr;           // �ndice de escrita/leitura (ponteiro circular)
} AllPassFilter;

// Estrutura principal do Reverb (4 Combs em paralelo + 2 All-Pass em série)
typedef struct {
    CombFilter    comb[REVERB_NUM_COMBS];
    AllPassFilter allpass[REVERB_NUM_ALLPASSES];
    Int16         wet_gain_Q15;  // Ganho do sinal molhado em Q15
    Int16         dry_gain_Q15;  // Ganho do sinal seco em Q15
} Reverb;

// Instância global do reverb (definida em reverb.c)
extern Reverb g_reverb;

// Inicialização do reverb.
// Os parâmetros em float são usados apenas na configuração; o processamento é todo em Q15.
void initReverb(Reverb* reverb,
                Uint16 comb_delay, float comb_gain,
                Uint16 ap_delay,   float ap_gain,
                float wet_level);

// Processa uma única amostra (Int16) pelo reverb
Int16 processReverbSample(Int16 input, Reverb* reverb);

// Processa um bloco de áudio (modo ping/pong com DMA)
void processAudioReverb(Uint16* rxBlock, Uint16* txBlock);

void clearReverbState(void);

#endif /* REVERB_H_ */
