//////////////////////////////////////////////////////////////////////////////
// dma.h - Header file for DMA audio functions
//////////////////////////////////////////////////////////////////////////////

#ifndef DMA_H_
#define DMA_H_

#include "ezdsp5502.h"

// Buffer definitions
#define AUDIO_BUFFER_SIZE 1024
#define AUDIO_BLOCK_SIZE (AUDIO_BUFFER_SIZE / 2)

// Function prototypes
void configAudioDma(void);
void startAudioDma(void);
void stopAudioDma(void);

// External variables
extern volatile Uint16 dmaPingPongFlag;

#endif /* DMA_H_ */