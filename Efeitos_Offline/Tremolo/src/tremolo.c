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

Int16 tremoloProcess(Int16 xinp, tremolo *t)
{
	float xin;
	float yout;
	float m;

	xin = 0.7*xinp;
	m = (float)t->mod * t->A;
	yout = (m + 1)*xin;

	return rounding16 (yout);
}

#define W (2.0*PI*FR/FS)
void tremoloSweep(tremolo *t)
{
	t->n++;
	t->mod = sin(W*t->n);
}
