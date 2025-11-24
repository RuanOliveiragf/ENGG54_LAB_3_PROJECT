/*
* flangerTest.c
*
*  Created on: Oct 27, 2012
*      Author: BLEE
*
*  Description: This is the test program for floating-point flangering effect experiment
*
*  For the book "Real Time Digital Signal Processing:
*                Fundamentals, Implementation and Application, 3rd Ed"
*                By Sen M. Kuo, Bob H. Lee, and Wenshun Tian
*                Publisher: John Wiley and Sons, Ltd
*
*/

#include <stdlib.h>
#include <stdio.h>
#include "tistdtypes.h"
#include "flanger.h"

Int16  d_buffer[MAX_BUF_SIZE];
Int8   temp[2*MAX_BUF_SIZE];
Uint8  waveHeader[44];

void wHeader(Uint8 *w, float f1, float f2, Uint32 bytes);

float depth = 1.0;
flanger param;
flanger *t;

void main()
{
    FILE   *fpIn,*fpOut;
    Int16  i,j,yOut,c;
    Uint32 cnt;

    //  Open data files
    printf("Enter 1 for using PCM file, enter 2 for using WAV file\n");
    scanf ("%d", &c);

    if (c == 2)
    {
    	fpIn = fopen("..\\data\\original.wav", "rb");
    	fpOut = fopen("..\\data\\audioOut.wav", "wb");
    }
    else
    {
    	fpIn = fopen("..\\data\\Soxphone8kHz.pcm", "rb");
    	fpOut = fopen("..\\data\\audioOut.pcm", "wb");
    }
    if (fpIn == NULL)
    {
        printf( "Problem opening the input audio file\n" );
        exit(0);
    }

    if (c == 2)
    {
      fread(waveHeader, sizeof(Int8), 44, fpIn);		// Advance input to speech data
      fwrite(waveHeader, sizeof(Int8), 44, fpOut);		// Write header for output file
    }

    printf("Exp --- flanger experiment\n");

    /* initialization */
    t = &param;
    flangerInit(depth, t);

    // read initial buffer
    fread(&temp, sizeof(Int8), 2*MAX_BUF_SIZE, fpIn);
	j = 0;
    for(i=MAX_BUF_SIZE-1; i> 0; i--)
    {
        d_buffer[i] = temp[j++]&0xFF;
        d_buffer[i] |= temp[j++]<<8;
    }
    cnt = 0;

    /*the latest is in [0]*/
    while ((fread(&temp, sizeof(Int8),2*MAX_BUF_SIZE, fpIn)) == 2*MAX_BUF_SIZE )
    {
        for (j=0, i=0; i<MAX_BUF_SIZE; i++)
        {
            d_buffer[0] = (temp[j]&0xFF)|(temp[j+1]<<8);
            /* apply the Tremolo_process function to the sample */
            yOut = Flanger_process(d_buffer[0], t);
            Flanger_sweep(t);
            t->n++;

            /* save the output value */
            temp[j++] = yOut&0xFF;
            temp[j++] = (yOut>>8)&0xFF;
        }

        fwrite(&temp, sizeof(Int8), 2*MAX_BUF_SIZE, fpOut);
        cnt += MAX_BUF_SIZE;
        printf("%ld data samples processed\n", cnt);
    }

    if (c == 2)		// based on input WAVE file to generate output
    {
      wHeader(waveHeader, 8000, 8000, cnt<<1);
      rewind(fpOut);
      fwrite(waveHeader, sizeof(Int8), 44, fpOut);
    }

    fclose(fpOut);
    fclose(fpIn);
    printf("Exp --- completed\n");
}

// Create the wave file header
void wHeader(Uint8 *w, float f1, float f2, Uint32 bytes)
{
    Int32 t;

    t = bytes;
    w[40] = (t>>0)&0xff;	// Set up output file size
    w[41] = (t>>8)&0xff;
    w[42] = (t>>16)&0xff;
    w[43] = (t>>24)&0xff;
    t += 36;
    w[4] = (t>>0)&0xff;
    w[5] = (t>>8)&0xff;
    w[6] = (t>>16)&0xff;
    w[7] = (t>>24)&0xff;
    t = w[24]|(w[25]<<8)|((Int32)w[26]<<16)|((Int32)w[27]<<24);
    t = (Int32)((float)t*f2/f1);
    w[24] = (t>>0)&0xff;	// Set up output file frequency
    w[25] = (t>>8)&0xff;
    w[26] = (t>>16)&0xff;
    w[27] = (t>>24)&0xff;
    t = w[28]|(w[29]<<8)|((Int32)w[30]<<16)|((Int32)w[31]<<24);
    t = (Int32)((float)t*f2/f1);
    w[28] = (t>>0)&0xff;	// Set up output file byte rate
    w[29] = (t>>8)&0xff;
    w[30] = (t>>16)&0xff;
    w[31] = (t>>24)&0xff;
    return;
}

