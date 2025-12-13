//////////////////////////////////////////////////////////////////////////////
// flanger.h - Efeito Flanger (Parâmetros idênticos ao Python)
//////////////////////////////////////////////////////////////////////////////

#ifndef FLANGER_H_
#define FLANGER_H_

#include "tistdtypes.h"

// Configurações do Buffer
#define FLANGER_DELAY_SIZE 512
#define LFO_SIZE 256

// Parâmetros calculados para 48kHz (Python: 1ms a 5ms, 0.7 Gain)
// Min: 48 samples, Max: 240 samples.
#define FLANGER_L0 144              // Média ((240+48)/2)
#define FLANGER_A 96                // Amplitude ((240-48)/2)
#define FLANGER_G 22938             // Ganho 0.7 em Q15 (0x599A)

// Incremento de fase para 0.5Hz @ 48kHz
// Inc = (0.5 * 2^32) / 48000 = 44739
#define LFO_INC 44739

// Variáveis globais
extern Int16 g_flangerBuffer[FLANGER_DELAY_SIZE];
extern Int16 g_lfoTable[LFO_SIZE];

extern volatile Uint16 g_flangerWriteIndex;
extern volatile Uint32 g_flangerPhaseAcc;
extern volatile Uint32 g_flangerPhaseInc;

// Funções
void initFlanger(void);
void processAudioFlanger(Uint16* rxBlock, Uint16* txBlock, Uint16 blockSize);
void clearFlanger(void);

#endif /* FLANGER_H_ */
