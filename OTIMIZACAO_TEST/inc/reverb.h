//////////////////////////////////////////////////////////////////////////////
// reverb.h - Efeito Reverb (Schroeder)
//////////////////////////////////////////////////////////////////////////////

#ifndef REVERB_H_
#define REVERB_H_

#include "tistdtypes.h"

// Configuração do Reverb
#define REVERB_NUM_COMBS       4
#define REVERB_NUM_ALLPASSES   2
#define REVERB_COMB_CHUNK      1200u  // REDUZIDO para economizar memória
#define REVERB_AP_CHUNK        240u   // REDUZIDO

// Estruturas de Filtro
typedef struct {
    Int16* buffer;
    Uint16 delay_samples;
    Int16  gain_Q15;
    Uint16 ptr;
} CombFilter;

typedef struct {
    Int16* buffer;
    Uint16 delay_samples;
    Int16  gain_Q15;
    Uint16 ptr;
} AllPassFilter;

typedef struct {
    CombFilter    comb[REVERB_NUM_COMBS];
    AllPassFilter allpass[REVERB_NUM_ALLPASSES];
    Int16         wet_gain_Q15;
    Int16         dry_gain_Q15;
} Reverb;

// Variável global do reverb
extern Reverb g_reverb;

// Funções do Reverb
void initReverb(void);
void processAudioReverb(Uint16* rxBlock, Uint16* txBlock, Uint16 blockSize);
void clearReverb(void);

#endif /* REVERB_H_ */