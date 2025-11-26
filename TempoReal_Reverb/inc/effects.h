//////////////////////////////////////////////////////////////////////////////
// effects.h - Header para efeitos de áudio
//////////////////////////////////////////////////////////////////////////////

#ifndef EFFECTS_H_
#define EFFECTS_H_

#include "ezdsp5502.h"
#include "tistdtypes.h"

// Definições para o Flanger
#define FLANGER_DELAY_SIZE 1024
#define FLANGER_DELAY_MASK (FLANGER_DELAY_SIZE - 1)
#define LFO_SIZE 256
#define FLANGER_L0 100      // Atraso médio em amostras
#define FLANGER_A 50        // Amplitude do LFO em amostras
#define FLANGER_fr 0.5f     // Frequência do LFO em Hz
#define FLANGER_g 0x4000    // Ganho do flanger em Q15 (0.5)

// Variáveis globais para efeitos (flanger)
extern Int16 g_flangerBuffer[FLANGER_DELAY_SIZE];
extern volatile Uint16 g_flangerWriteIndex;
extern Int16 g_lfoTable[LFO_SIZE];
extern volatile Uint16 g_lfoIndex;
extern float g_lfoPhaseInc;

// Protótipos das funções
void initFlanger(void);
void processAudioFlanger(Uint16* rxBlock, Uint16* txBlock);
void processAudioLoopback(Uint16* rxBlock, Uint16* txBlock);
void clearFlangerState(void);

// Tremolo
void initTremoloEffect(void);
void processAudioTremolo(Uint16* rxBlock, Uint16* txBlock);
void resetTremoloState(void);

// Reverb
void initReverbEffect(void);
void processAudioReverb(Uint16* rxBlock, Uint16* txBlock);

#endif /* EFFECTS_H_ */
