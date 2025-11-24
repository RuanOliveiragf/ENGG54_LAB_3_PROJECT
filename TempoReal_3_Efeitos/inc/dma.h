//////////////////////////////////////////////////////////////////////////////
// dma.h - Header file for DMA audio functions
//////////////////////////////////////////////////////////////////////////////

#ifndef DMA_H_
#define DMA_H_

#include "ezdsp5502.h"

// Buffer definitions
#define AUDIO_BUFFER_SIZE 4096
#define AUDIO_BLOCK_SIZE (AUDIO_BUFFER_SIZE / 2)

// Estados para seleção de efeitos
#define EFFECT_LOOPBACK 0
#define EFFECT_FLANGER  1
#define EFFECT_TREMOLO  2
#define EFFECT_REVERB   3   // <- novo efeito reverb

// Function prototypes
void configAudioDma(void);
void startAudioDma(void);
void stopAudioDma(void);
void manageEffectTransition(Uint8 currentEffect);

// External variables
extern volatile Uint16 dmaPingPongFlag;
extern volatile Uint8 currentEffect;  // Para selecionar efeitos

#endif /* DMA_H_ */
