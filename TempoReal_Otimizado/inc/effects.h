//////////////////////////////////////////////////////////////////////////////
// effects.h - Header para efeitos de áudio
//////////////////////////////////////////////////////////////////////////////

#ifndef EFFECTS_H_
#define EFFECTS_H_

#include "ezdsp5502.h"
#include "tistdtypes.h"

// Definições para o Flanger (BUFFERS REDUZIDOS)
#define FLANGER_DELAY_SIZE 256       // Reduzido de 1024 para 256
#define FLANGER_DELAY_MASK (FLANGER_DELAY_SIZE - 1)
#define LFO_SIZE 128                 // Reduzido de 256 para 128
#define FLANGER_L0 50                // Atraso médio reduzido
#define FLANGER_A 25                 // Amplitude reduzida
#define FLANGER_fr 0.5f              // Frequência do LFO em Hz
#define FLANGER_g 0x4000             // Ganho do flanger em Q15 (0.5)

// Estrutura para estado do flanger (agora local)
typedef struct {
    Int16 buffer[FLANGER_DELAY_SIZE];
    Uint16 writeIndex;
    Int16 lfoTable[LFO_SIZE];
    Uint16 lfoIndex;
} FlangerState;

// Estrutura para estado do tremolo
typedef struct {
    float mod;
    float A;
    Int32 n;
} TremoloState;

// Defini��o da estrutura do Tremolo para suportar o ponteiro "tremolo *t"
typedef struct {
    float mod;      // Valor atual da modula��o
    unsigned long n; // Contador de amostras/tempo
} tremolo_t;


// Protótipos das funções
void initEffect(Uint8 effect);
void cleanupEffect(Uint8 oldEffect);
void processAudioFlanger(Uint16* rxBlock, Uint16* txBlock);
void processAudioLoopback(Uint16* rxBlock, Uint16* txBlock);
void processAudioTremolo(Uint16* rxBlock, Uint16* txBlock);
void tremoloSweep(tremolo_t *t);
Int16 tremoloProcess(Int16 xin, tremolo_t *t);

#endif /* EFFECTS_H_ */
