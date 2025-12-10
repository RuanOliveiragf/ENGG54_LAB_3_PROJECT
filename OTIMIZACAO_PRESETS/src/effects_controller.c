//////////////////////////////////////////////////////////////////////////////
// effects_controller.c - Implementação do Sistema de Controle
//////////////////////////////////////////////////////////////////////////////

#include "effects_controller.h"
#include "flanger.h"
#include "tremolo.h"
#include "reverb.h"
#include <string.h>
#include "pitch_shift.h"

// Controlador global
EffectController g_effectController;
volatile Uint8 currentEffect = EFFECT_LOOPBACK;

// Inicialização do controlador
void initEffectController(void)
{
    int i;
    
    // Configura informações dos efeitos
    g_effectController.effects[EFFECT_LOOPBACK].name = "LOOPBACK";
    g_effectController.effects[EFFECT_LOOPBACK].code = EFFECT_LOOPBACK;
    
    g_effectController.effects[EFFECT_FLANGER].name = "FLANGER ";
    g_effectController.effects[EFFECT_FLANGER].code = EFFECT_FLANGER;
    
    g_effectController.effects[EFFECT_TREMOLO].name = "TREMOLO ";
    g_effectController.effects[EFFECT_TREMOLO].code = EFFECT_TREMOLO;
    
    g_effectController.effects[EFFECT_REVERB].name = "REVERB  ";
    g_effectController.effects[EFFECT_REVERB].code = EFFECT_REVERB;
    
    // Estado inicial
    g_effectController.currentEffect = EFFECT_LOOPBACK;
    g_effectController.pitchShiftActive = 0; // Começa desativado
    
    for (i = 0; i < EFFECT_COUNT; i++) {
        g_effectController.effectInitialized[i] = 0;
        g_effectController.effectActive[i] = 0;
    }
    
    // Loopback sempre ativo e inicializado
    g_effectController.effectActive[EFFECT_LOOPBACK] = 1;
    g_effectController.effectInitialized[EFFECT_LOOPBACK] = 1;
    
    currentEffect = EFFECT_LOOPBACK;
}

// Configura efeito ativo
void setEffect(Uint8 effect)
{
    if (effect >= EFFECT_COUNT) {
        effect = EFFECT_LOOPBACK;
    }
    
    Uint8 oldEffect = g_effectController.currentEffect;
    
    // Limpa efeito anterior se necessário
    if (oldEffect != effect && oldEffect != EFFECT_LOOPBACK) {
        cleanupEffect(oldEffect);
    }
    
    // Inicializa novo efeito se necessário
    if (!g_effectController.effectInitialized[effect]) {
        switch (effect) {
            case EFFECT_FLANGER:
                initFlanger();
                g_effectController.effectInitialized[effect] = 1;
                break;
                
            case EFFECT_TREMOLO:
                initTremolo();
                g_effectController.effectInitialized[effect] = 1;
                break;
                
            case EFFECT_REVERB:
                initReverb();
                g_effectController.effectInitialized[effect] = 1;
                break;
                
            default:
                // Loopback já está inicializado
                break;
        }
    }
    
    // Ativa o novo efeito
    g_effectController.currentEffect = effect;
    g_effectController.effectActive[effect] = 1;
    currentEffect = effect;
}

// Configura estado do Pitch Shift
void setPitchShiftEnabled(Uint8 enabled)
{
    // Se estava desligado e vai ligar, inicializa para limpar buffers
    if (enabled && !g_effectController.pitchShiftActive) {
        initPitchShift();
        // CUIDADO: initPitchShift reseta a frequência para Default (1.0x).
        // Quem chamar esta função deve setar a frequência DEPOIS.
    }
    g_effectController.pitchShiftActive = enabled;
}

// Retorna se Pitch Shift está ativo
Uint8 isPitchShiftEnabled(void)
{
    return g_effectController.pitchShiftActive;
}

// Obtém próximo efeito na sequência
Uint8 getNextEffect(Uint8 current)
{
    Uint8 next = (current + 1) % EFFECT_COUNT;
    return next;
}

// Limpa recursos de um efeito específico
void cleanupEffect(Uint8 effect)
{
    if (effect >= EFFECT_COUNT) return;
    
    g_effectController.effectActive[effect] = 0;
    
    switch (effect) {
        case EFFECT_FLANGER:
            clearFlanger();
            break;
            
        case EFFECT_TREMOLO:
            // Tremolo não tem buffers grandes para limpar
            break;
            
        case EFFECT_REVERB:
            clearReverb();
            break;
            
        default:
            // Loopback não precisa limpar
            break;
    }
}

// Limpa todos os efeitos (exceto loopback)
void cleanupAllEffects(void)
{
    int i;
    
    for (i = 1; i < EFFECT_COUNT; i++) {
        cleanupEffect(i);
    }
    
    // Garante que loopback está ativo
    setEffect(EFFECT_LOOPBACK);
}

// Obtém efeito atual
Uint8 getCurrentEffect(void)
{
    return g_effectController.currentEffect;
}

// Obtém nome do efeito
const char* getEffectName(Uint8 effect)
{
    if (effect >= EFFECT_COUNT) {
        return "UNKNOWN";
    }
    return g_effectController.effects[effect].name;
}
