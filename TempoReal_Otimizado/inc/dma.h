//////////////////////////////////////////////////////////////////////////////
// dma.h - Header file for DMA audio functions
//////////////////////////////////////////////////////////////////////////////

#ifndef DMA_H_
#define DMA_H_

#include "ezdsp5502.h"

// Buffer definitions (REDUZIDOS)
#define AUDIO_BUFFER_SIZE 512        // Reduzido de 4096 para 512
#define AUDIO_BLOCK_SIZE (AUDIO_BUFFER_SIZE / 2)

// Estados para seleção de efeitos
#define EFFECT_LOOPBACK 0
#define EFFECT_FLANGER  1
#define EFFECT_TREMOLO  2

// Function prototypes
void configAudioDma(void);
void startAudioDma(void);
void stopAudioDma(void);

// External variables
extern volatile Uint16 dmaPingPongFlag;
extern volatile Uint8 currentEffect;

#endif /* DMA_H_ */