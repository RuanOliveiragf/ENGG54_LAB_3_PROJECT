//////////////////////////////////////////////////////////////////////////////
// dma.h - Header file for DMA audio functions
//////////////////////////////////////////////////////////////////////////////

#ifndef DMA_H_
#define DMA_H_

#include "ezdsp5502.h"

// Buffer definitions
#define AUDIO_BUFFER_SIZE 512
#define AUDIO_BLOCK_SIZE (AUDIO_BUFFER_SIZE / 2)


// Function prototypes
void configAudioDma(void);
void startAudioDma(void);
void stopAudioDma(void);
void manageEffectTransition(Uint8 currentEffect);

// External variables
extern volatile Uint16 dmaPingPongFlag;
extern volatile Uint8 currentEffect;  // Para selecionar efeitos

#endif /* DMA_H_ */
