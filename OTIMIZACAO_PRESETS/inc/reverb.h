//////////////////////////////////////////////////////////////////////////////
// reverb.h - Efeito Reverb Stereo
//////////////////////////////////////////////////////////////////////////////

#ifndef REVERB_H_
#define REVERB_H_

#include "tistdtypes.h"

// Configuração
#define REVERB_NUM_COMBS       4
#define REVERB_NUM_ALLPASSES   2

// TAMANHO TOTAL DA MEMÓRIA DO REVERB
// Hall requer aprox 13.000 words para Stereo.
// Alocamos 16.000 words (32KB) para ter margem, cabendo na DARAM (64KB).
#define REVERB_MEM_SIZE        32000u

// Spread de 23 amostras (~0.5ms) para o canal direito
#define REVERB_SPREAD          23u

// Estruturas de Filtro
typedef struct {
    Int16* buffer;          // Ponteiro para o início do buffer deste filtro
    Uint16 delay_samples;   // Tamanho real do delay
    Int16  gain_Q15;
    Uint16 ptr;
} CombFilter;

typedef struct {
    Int16* buffer;
    Uint16 delay_samples;
    Int16  gain_Q15;
    Uint16 ptr;
} AllPassFilter;

// Núcleo de processamento de UM canal
typedef struct {
    CombFilter    comb[REVERB_NUM_COMBS];
    AllPassFilter allpass[REVERB_NUM_ALLPASSES];
} ReverbCore;

// Estrutura Principal Stereo
typedef struct {
    ReverbCore    left;           // Canal Esquerdo
    ReverbCore    right;          // Canal Direito
    Int16         wet_gain_Q15;

    // Gerenciamento de memória simples
    Uint16        memAllocated;
} Reverb;

// Presets
#define REVERB_PRESET_HALL    0
#define REVERB_PRESET_ROOM    1
#define REVERB_PRESET_STAGE   2
#define REVERB_PRESET_COUNT   3

typedef Uint8 ReverbPreset;

extern Reverb       g_reverb;
extern ReverbPreset g_reverbPreset;

// Funções
void initReverb(void);
void processAudioReverb(Uint16* rxBlock, Uint16* txBlock, Uint16 blockSize);
void clearReverb(void);
void setReverbPreset(ReverbPreset preset);

#endif /* REVERB_H_ */
