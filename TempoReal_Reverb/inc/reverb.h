/////////////////////////////////////////////////////////////////////////////
// reverb.h - Header atualizado para configuração estilo Python
/////////////////////////////////////////////////////////////////////////////

#ifndef REVERB_H_
#define REVERB_H_

#include "tistdtypes.h"

// Quantidade de filtros (Schroeder padrão)
#define REVERB_NUM_COMBS       4
#define REVERB_NUM_ALLPASSES   2

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

extern Reverb g_reverb;

void initReverb(Reverb* reverb,
                float* delays_combs_ms, float* gains_combs,
                float* delays_ap_ms,    float* gains_ap,
                float wet_level);

Int16 processReverbSample(Int16 input, Reverb* reverb);
void processAudioReverb(Uint16* rxBlock, Uint16* txBlock);
void clearReverbState(void);

#endif /* REVERB_H_ */
