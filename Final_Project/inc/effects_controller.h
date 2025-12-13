//////////////////////////////////////////////////////////////////////////////
// effects_controller.h - Sistema Central de Controle de Efeitos
//////////////////////////////////////////////////////////////////////////////

#ifndef EFFECTS_CONTROLLER_H_
#define EFFECTS_CONTROLLER_H_

#include "tistdtypes.h"

// Definições de efeitos
#define EFFECT_LOOPBACK  0
#define EFFECT_FLANGER   1
#define EFFECT_TREMOLO   2
#define EFFECT_REVERB    3
#define EFFECT_COUNT     4

// Nomes dos efeitos (para display)
typedef struct {
    const char* name;
    Uint8 code;
} EffectInfo;

// Sistema de controle
typedef struct {
    Uint8 currentEffect;
    // Flag para ativar o estágio de Pitch Shift (Pré-processamento)
    Uint8 pitchShiftActive;
    Uint8 effectInitialized[EFFECT_COUNT];
    Uint8 effectActive[EFFECT_COUNT];
    EffectInfo effects[EFFECT_COUNT];
} EffectController;

// Variável global do controlador
extern EffectController g_effectController;
extern volatile Uint8 currentEffect;  // Compatibilidade

// Funções do controlador
void initEffectController(void);
void setEffect(Uint8 effect);

// --- Controle do Pitch Shift ---
void setPitchShiftEnabled(Uint8 enabled);
Uint8 isPitchShiftEnabled(void);

Uint8 getNextEffect(Uint8 current);
void cleanupEffect(Uint8 effect);
void cleanupAllEffects(void);
Uint8 getCurrentEffect(void);
const char* getEffectName(Uint8 effect);

#endif /* EFFECTS_CONTROLLER_H_ */
