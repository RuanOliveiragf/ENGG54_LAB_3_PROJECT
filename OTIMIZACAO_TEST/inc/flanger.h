//////////////////////////////////////////////////////////////////////////////
// flanger.h - Efeito Flanger
//////////////////////////////////////////////////////////////////////////////

#ifndef FLANGER_H_
#define FLANGER_H_

#include "tistdtypes.h"

// Configurações do Flanger (otimizadas para memória)
#define FLANGER_DELAY_SIZE 512      // Buffer reduzido
#define LFO_SIZE 128                // Tabela LFO reduzida
#define FLANGER_L0 80               // Delay médio
#define FLANGER_A 30                // Amplitude LFO
#define FLANGER_fr 0.5f             // Frequência LFO (Hz)
#define FLANGER_g 0x4000            // Ganho (0.5 em Q15)

// Variáveis globais (alocadas na seção effectsMem)
extern Int16 g_flangerBuffer[FLANGER_DELAY_SIZE];
extern Int16 g_lfoTable[LFO_SIZE];
extern volatile Uint16 g_flangerWriteIndex;
extern volatile Uint16 g_lfoIndex;

// Funções do Flanger
void initFlanger(void);
void processAudioFlanger(Uint16* rxBlock, Uint16* txBlock, Uint16 blockSize);
void clearFlanger(void);

#endif /* FLANGER_H_ */