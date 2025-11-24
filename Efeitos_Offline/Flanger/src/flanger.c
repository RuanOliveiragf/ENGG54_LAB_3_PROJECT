/*
* flanger.c
*
*  Created on: Oct 27, 2012
*      Author: BLEE
*
*  Description: This file contains functions used for flangering effect experiment
*
*  For the book "Real Time Digital Signal Processing:
*                Fundamentals, Implementation and Application, 3rd Ed"
*                By Sen M. Kuo, Bob H. Lee, and Wenshun Tian
*                Publisher: John Wiley and Sons, Ltd
*
*/

#include <math.h>
#include "tistdtypes.h"
#include "flanger.h"
 
void flangerInit(float depth, flanger *t)
{
    t->delay = 100; 			// Average delay
    t->A_maxSwing = 0.5;		// Max swing
    t->G_depth = depth;			// Depth
    t->n = 0;
}

/*This is the flanging process task  that uses the delay task inside*/
Int16 Flanger_process(Int16 xinp, flanger *t)
{
	float y0, y1;
	float fracY,yout;
	Int16 iy0, iy1;
	Int16 i;

	iy0 = (Int16)(t->var_delay);		// Latest
	iy1 = (Int16)(t->var_delay +1.0);	// History

	/* Two points linear interpolation */
	y0 = (float)d_buffer[iy0];
	y1 = (float)d_buffer[iy1];
	fracY = (float)(1.0 - (t->var_delay - (float)iy0));

	yout = fracY*y0 + (1.0-fracY)*y1;

	/* Apply Sweep depth and normalize*/
	yout = xinp + t->G_depth * yout ;
	yout = yout / (1.0 + t->G_depth);

	/* Rotate buffer ready for next new sample in */
	for(i=MAX_BUF_SIZE-1; i> 0; i--)
	{
		d_buffer[i ] = d_buffer[i-1];
	}
	return (Int16)yout;
}

/* this sweep function creates a low frequency oscillator*/
#define W (2.0*3.1415926*LFO_F/8000.0)
void Flanger_sweep(flanger *t)
{
	t->var_delay = t->delay*(1.0 + t->A_maxSwing*sin(W*t->n));
}
