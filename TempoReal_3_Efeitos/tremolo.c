/*
* tremolo.c
*
*  Created on: Oct 28, 2012
*      Author: BLEE
*
*  Description: This program contains functions for floating-point tremolo effect experiment
*
*  For the book "Real Time Digital Signal Processing:
*                Fundamentals, Implementation and Application, 3rd Ed"
*                By Sen M. Kuo, Bob H. Lee, and Wenshun Tian
*                Publisher: John Wiley and Sons, Ltd
*
*/

#include <stdio.h> 
#include <math.h>
#include "tistdtypes.h"
#include "tremolo.h"

#define W (2.0*PI*FR/FS)
 
void tremoloInit(float depth, tremolo *t)
{
	t->A     = depth;
	t->mod     = 0.0;
	t->n       = 0;
	printf( "tremoloInit() at depth [%f]\n",t->A);
}
 
// Round to short
Int16 rounding16(float xd)
{
	Int16 y16;
	
	xd += 0.5;

	if(xd > 32767.0) y16 = 32767;
	else if (xd < -32768.0) y16 = -32768;
	else y16 = (Int16) xd;

	return y16;
}

/*
 * tremoloProcess( )
 *
 * Apply Tremolo effect to a sample
 */
Int16 tremoloProcess(Int16 xin, tremolo *t)
{
    float m;
    float gain;
    float temp_out; // Usamos float para o cálculo intermediário seguro
    Int16 yout;

    // Calcula o fator de modulação (normaliza para 0.0 a 1.0)
    m = (float)t->mod / 32768.0;

    // Calcula o ganho: 1 - depth * modulation
    // (Isso cria a oscilação de volume)
    gain = 1.0 - t->A * m;

    // Aplica o ganho ao sinal de entrada
    temp_out = (float)xin * gain;

    // === PROTEÇÃO DE SATURAÇÃO (Safety Limiter) ===
    // Garante que, se o resultado flutuante exceder os limites de 16 bits,
    // ele seja travado no máximo/mínimo em vez de "dar a volta" (ruído).
    if (temp_out > 32767.0f) {
        yout = 32767;
    } else if (temp_out < -32768.0f) {
        yout = -32768;
    } else {
        yout = (Int16)temp_out;
    }

    return yout;
}


void tremoloSweep(tremolo *t)
{
	t->n++;
	t->mod = sin(W*t->n);
}
