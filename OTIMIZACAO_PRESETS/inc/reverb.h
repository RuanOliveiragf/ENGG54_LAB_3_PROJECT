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
    Uint16 delay_samples;   // Número de amostras de atraso
    Int16  gain_Q15;        // Ganho em Q15
    Uint16 ptr;             // Índice do buffer circular
} CombFilter;

typedef struct {
    Int16* buffer;
    Uint16 delay_samples;   // Número de amostras de atraso
    Int16  gain_Q15;        // Ganho em Q15
    Uint16 ptr;             // Índice do buffer circular
} AllPassFilter;

typedef struct {
    CombFilter    comb[REVERB_NUM_COMBS];
    AllPassFilter allpass[REVERB_NUM_ALLPASSES];
    Int16         wet_gain_Q15;   // ganho da parte reverberada (Q15)
    Int16         dry_gain_Q15;   // ganho do sinal direto (Q15)
} Reverb;

// -------------------- Presets de Reverb --------------------

// IDs de preset
#define REVERB_PRESET_HALL    0   // "REV-HALL"
#define REVERB_PRESET_ROOM    1   // "REV-ROOM"
#define REVERB_PRESET_STAGE   2   // "REV-STAGE"
#define REVERB_PRESET_COUNT   3

typedef Uint8 ReverbPreset;

// Variáveis globais
extern Reverb       g_reverb;
extern ReverbPreset g_reverbPreset;

// Funções do Reverb
void initReverb(void);
void processAudioReverb(Uint16* rxBlock, Uint16* txBlock, Uint16 blockSize);
void clearReverb(void);

// Controle de preset
void setReverbPreset(ReverbPreset preset);
ReverbPreset getReverbPreset(void);

#endif /* REVERB_H_ */
