#ifndef __TREMOLO_H__
#define __TREMOLO_H__

#include "tistdtypes.h"

// Tamanho da tabela (256 pontos)
#define SINE_TABLE_SIZE 256

typedef struct {
    Int16  current_val; // Valor atual do oscilador (Q15)
    Int16  depth;       // Profundidade (Q15: 0 a 32767)

    // Controle de Fase para DDS (Direct Digital Synthesis)
    Uint32 phase_acc;   // Acumulador de fase de 32 bits
    Uint32 phase_inc;   // Incremento de fase por amostra
} tremolo;

// Funções atualizadas
void  tremoloInit(Int16 depth_q15, tremolo *t);
Int16 tremoloProcess(Int16 xin, tremolo *t);
void  tremoloSweep(tremolo *t);
 
#endif
