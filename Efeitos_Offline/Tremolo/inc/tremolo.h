/*
* tremolo.h
*
*  Created on: Oct 27, 2012
*      Author: BLEE
*
*  Description: This C header file for tremolo experiment
*
*  For the book "Real Time Digital Signal Processing:
*                Fundamentals, Implementation and Application, 3rd Ed"
*                By Sen M. Kuo, Bob H. Lee, and Wenshun Tian
*                Publisher: John Wiley and Sons, Ltd
*
*/

#ifndef __TREMOLO_H__
#define __TREMOLO_H__

#define MAX_BUF_SIZE 1024
#define PI 3.1415926
#define FS 8000
#define FR 1
#define TDEPTH 1.0;

typedef struct {
	float mod;
	float A;
	Int32 n;
} tremolo;

void  tremoloInit(float depth, tremolo *t);
Int16 tremoloProcess(Int16 xin, tremolo *t);
void  tremoloSweep(tremolo *t);
 
#endif
