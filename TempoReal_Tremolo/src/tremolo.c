#include "tistdtypes.h"
#include "tremolo.h"

// Constantes
#define FS 48000UL          // Frequência de amostragem
#define FR 3UL              // Frequência do Tremolo (3 Hz)

// Tabela de Seno em Q15 (Valores de -32767 a 32767)
#pragma DATA_ALIGN(sine_table, 4)
static const Int16 sine_table[SINE_TABLE_SIZE] = {
      0,   804,  1607,  2410,  3211,  4011,  4807,  5601,  6392,  7179,  7961,  8739,  9511, 10278, 11038, 11792,
  12539, 13278, 14009, 14732, 15446, 16150, 16845, 17530, 18204, 18867, 19519, 20159, 20787, 21402, 22004, 22594,
  23169, 23731, 24278, 24811, 25329, 25831, 26318, 26789, 27244, 27683, 28105, 28510, 28897, 29268, 29621, 29955,
  30272, 30571, 30851, 31113, 31356, 31580, 31785, 31970, 32137, 32284, 32412, 32520, 32609, 32678, 32727, 32757,
  32767, 32757, 32727, 32678, 32609, 32520, 32412, 32284, 32137, 31970, 31785, 31580, 31356, 31113, 30851, 30571,
  30272, 29955, 29621, 29268, 28897, 28510, 28105, 27683, 27244, 26789, 26318, 25831, 25329, 24811, 24278, 23731,
  23169, 22594, 22004, 21402, 20787, 20159, 19519, 18867, 18204, 17530, 16845, 16150, 15446, 14732, 14009, 13278,
  12539, 11792, 11038, 10278,  9511,  8739,  7961,  7179,  6392,  5601,  4807,  4011,  3211,  2410,  1607,   804,
      0,  -804, -1607, -2410, -3211, -4011, -4807, -5601, -6392, -7179, -7961, -8739, -9511, -10278, -11038, -11792,
 -12539, -13278, -14009, -14732, -15446, -16150, -16845, -17530, -18204, -18867, -19519, -20159, -20787, -21402, -22004, -22594,
 -23169, -23731, -24278, -24811, -25329, -25831, -26318, -26789, -27244, -27683, -28105, -28510, -28897, -29268, -29621, -29955,
 -30272, -30571, -30851, -31113, -31356, -31580, -31785, -31970, -32137, -32284, -32412, -32520, -32609, -32678, -32727, -32757,
 -32767, -32757, -32727, -32678, -32609, -32520, -32412, -32284, -32137, -31970, -31785, -31580, -31356, -31113, -30851, -30571,
 -30272, -29955, -29621, -29268, -28897, -28510, -28105, -27683, -27244, -26789, -26318, -25831, -25329, -24811, -24278, -23731,
 -23169, -22594, -22004, -21402, -20787, -20159, -19519, -18867, -18204, -17530, -16845, -16150, -15446, -14732, -14009, -13278,
 -12539, -11792, -11038, -10278,  -9511,  -8739,  -7961,  -7179,  -6392,  -5601,  -4807,  -4011,  -3211,  -2410,  -1607,  -804
};

void tremoloInit(Int16 depth_q15, tremolo *t)
{
    t->depth = depth_q15;
    t->current_val = 0;
    t->phase_acc = 0;

    // Cálculo do incremento de fase para síntese direta (DDS)
    // Inc = (Freq_Tremolo * 2^32) / Freq_Amostragem
    // Inc = (3 * 4294967296) / 48000 = 268435
    // Para 3Hz exatos:
    t->phase_inc = 268435;
}

/*
 * Processamento OTIMIZADO Q15
 * Sem floats, sem divisões.
 */
Int16 tremoloProcess(Int16 xin, tremolo *t)
{
    Int32 temp;
    Int16 mod = t->current_val; // Valor da tabela (-32767 a 32767)

    // Cálculo do Ganho:
    // Fórmula Python: gain = (1 - depth/2) + (depth/2 * mod)
    // Em Q15: 1.0 = 32767

    Int16 half_depth = t->depth >> 1;     // depth / 2
    Int16 offset = 32767 - half_depth;    // (1 - depth/2)

    // Parte variável: (depth/2 * mod)
    // Multiplicação Q15 * Q15 gera Q30. Pegamos os 15 bits superiores.
    Int16 variable = (Int16)(((Int32)half_depth * mod) >> 15);

    Int16 gain = offset + variable;

    // Aplica o ganho no sinal de entrada
    // Saída = (Entrada * Ganho) >> 15
    temp = (Int32)xin * gain;
    return (Int16)(temp >> 15);
}

void tremoloSweep(tremolo *t)
{
    // 1. Incrementa o acumulador de fase de 32 bits
    t->phase_acc += t->phase_inc;

    // 2. Pega os 8 bits mais significativos para usar como índice (0-255)
    // Deslocamos 24 bits para a direita (32 - 8 = 24)
    Uint16 index = (Uint16)(t->phase_acc >> 24);

    // 3. Busca na tabela
    t->current_val = sine_table[index];
}
