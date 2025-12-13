//////////////////////////////////////////////////////////////////////////////
// tremolo.h - Efeito Tremolo
//////////////////////////////////////////////////////////////////////////////

#ifndef TREMOLO_H_
#define TREMOLO_H_

#include "tistdtypes.h"

// Tamanho da tabela de seno
#define SINE_TABLE_SIZE 256

// Estrutura do Tremolo
typedef struct {
    Int16  current_val;     // Valor atual do oscilador (Q15)
    Int16  depth;           // Profundidade (Q15: 0 a 32767)
    Uint32 phase_acc;       // Acumulador de fase de 32 bits
    Uint32 phase_inc;       // Incremento de fase por amostra
} Tremolo;

// Variável global do tremolo
extern Tremolo g_tremolo;

// Funções do Tremolo
void initTremolo(void);
void processAudioTremolo(Uint16* rxBlock, Uint16* txBlock, Uint16 blockSize);

#endif /* TREMOLO_H_ */