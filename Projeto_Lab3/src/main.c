//////////////////////////////////////////////////////////////////////////////
// main.c - Sistema Principal com Controle de Efeitos
//////////////////////////////////////////////////////////////////////////////

#include "ezdsp5502.h"
#include "csl.h"
#include "ezdsp5502_mcbsp.h"
#include "ezdsp5502_i2cgpio.h"
#include "dma.h"
#include "timer.h"
#include "lcd.h"
#include "i2cgpio.h"
#include "isr.h"
#include "effects_controller.h"
#include "csl_chip.h"
#include "reverb.h"
#include "pitch_shift.h"

// DeclaraÃ§Ã£o do inÃ­cio da tabela de vetores em assembly
extern void VECSTART(void);

// Prototipo
void configPort(void);
void effectChangeFeedback(Uint8 effect);
void checkSwitch(void);
static void toggleLED(void);

// VariÃ¡veis globais
extern Uint16 timerFlag;
Uint8 ledNum   = 3;
Uint8 sw1State = 0;   // usado para o SW0 (timer)

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
void main(void)
{
    // 1. Configuracao centralizada do CSL
    CSL_init();
    IRQ_setVecs((Uint32)(&VECSTART));
    IRQ_globalDisable();

    // 2. InicializaÃ§Ãµes bÃ¡sicas
    initPLL();
    EZDSP5502_init();
    initLed();          // configura SW0, SW1 e LEDs
    configPort();       // selecao de BSP e reforca da config dos switches
 
    // 3. Inicializa o display OLED
    oled_start();

    // 4. Inicializa AIC3204 (codec de audio)
    initAIC3204();

    // 5. Inicializa controlador de efeitos
    initEffectController();
    initReverb();
    initPitchShift();

    // 6. Configura DMA de audio + Timer
    configAudioDma();
    initTimer0();

    // 7. Habilita interrupcoes globais e inicia DMA e Timer
    IRQ_globalEnable();
    startAudioDma();
    startTimer0();

    // Loop principal
    while (1)
    {
        // Pisca XF via timer
        if (timerFlag)
        {
            timerFlag = 0;
            toggleLED();
        }

        // Verifica botoes
        checkSwitch();
    }
}

// ---------------------------------------------------------------------------
// Configuracao de portas extras (multiplexadores etc.)
// ---------------------------------------------------------------------------
void configPort(void)
{
    EZDSP5502_I2CGPIO_configLine(BSP_SEL1, OUT);
    EZDSP5502_I2CGPIO_writeLine(BSP_SEL1, LOW);
    EZDSP5502_I2CGPIO_configLine(BSP_SEL1_ENn, OUT);
    EZDSP5502_I2CGPIO_writeLine(BSP_SEL1_ENn, LOW);

    // Garante que os botÃµes estÃ£o como entrada (reforÃ§o)
    EZDSP5502_I2CGPIO_configLine(SW0, IN);
    EZDSP5502_I2CGPIO_configLine(SW1, IN);
}

// ---------------------------------------------------------------------------
// Pisca XF e LED circular no timer
// ---------------------------------------------------------------------------
static void toggleLED(void)
{
    if (CHIP_FGET(ST1_55, XF))
        CHIP_FSET(ST1_55, XF, CHIP_ST1_55_XF_OFF);
    else
        CHIP_FSET(ST1_55, XF, CHIP_ST1_55_XF_ON);
}

// ---------------------------------------------------------------------------
// Feedback visual de efeito (LEDs)
// ---------------------------------------------------------------------------
void effectChangeFeedback(Uint8 effect)
{
    Uint8 led = effect % 4;     // LED0..LED3
    volatile Uint16 i;

    // Acende o LED (ativo em LOW)
    EZDSP5502_I2CGPIO_writeLine(LED0 + led, LOW);
    for (i = 0; i < 50000; i++);
    EZDSP5502_I2CGPIO_writeLine(LED0 + led, HIGH);
}

// ---------------------------------------------------------------------------
// Leitura e tratativa dos botoes SW0/SW1
// ---------------------------------------------------------------------------
void checkSwitch(void)
{
    static Uint8 lastEffectButtonState = 1;  // estado anterior de SW1 (para borda)
    static Uint8 effectStep = 0;            // 0..8, controla efeitos + presets

    Uint8 sw0Raw;
    Uint8 sw1Raw;

    // botoes (ativos em nivel baixo)
    sw0Raw = EZDSP5502_I2CGPIO_readLine(SW0);
    sw1Raw = EZDSP5502_I2CGPIO_readLine(SW1);

    // --- SW0: controla o timer (pressiona -> alterna peri­odo)
    if (sw0Raw == 0)
    {
        if (sw1State)          // apenas na transicoes
        {
            changeTimer();
            sw1State = 0;
        }
    } else {
        sw1State = 1;
    }

    // --- SW1: muda efeito / preset (deteccao de borda 1 -> 0)
    if ((sw1Raw == 0) && (lastEffectButtonState == 1))
    {
        effectStep = (effectStep + 1u) % 9u; // 0..8

        switch (effectStep)
        {
            case 0: // LOOPBACK
                setPitchShiftEnabled(0);
                setEffect(EFFECT_LOOPBACK);
                break;

            case 1: // REVERB HALL
                setPitchShiftEnabled(0);
                setReverbPreset(REVERB_PRESET_HALL);
                setEffect(EFFECT_REVERB);
                break;

            case 2: // REVERB ROOM 2
                setPitchShiftEnabled(0);
                setReverbPreset(REVERB_PRESET_ROOM);
                setEffect(EFFECT_REVERB);
                break;

            case 3: // REVERB STAGE + PITCH SHIFT (B)
                setPitchShiftEnabled(1);
                setPitchFrequency(493.88f);
                setReverbPreset(REVERB_PRESET_STAGE);
                setEffect(EFFECT_REVERB);
                break;

            case 4: // REVERB STAGE + PITCH SHIFT (D)
                setPitchShiftEnabled(1);
                setPitchFrequency(293.66f);
                setReverbPreset(REVERB_PRESET_STAGE);
                setEffect(EFFECT_REVERB);
                break;

            case 5: // REVERB STAGE + PITCH SHIFT (F)
                setPitchShiftEnabled(1);
                setPitchFrequency(349.23f);
                setReverbPreset(REVERB_PRESET_STAGE);
                setEffect(EFFECT_REVERB);
                break;

            case 6: // REVERB STAGE + PITCH SHIFT (Gb)
                setPitchShiftEnabled(1);
                setPitchFrequency(369.99f);
                setReverbPreset(REVERB_PRESET_STAGE);
                setEffect(EFFECT_REVERB);
                break;

            case 7: // FLANGER
                setPitchShiftEnabled(0);
                setEffect(EFFECT_FLANGER);
                break;

            case 8: // TREMOLO
                setPitchShiftEnabled(0);
                setEffect(EFFECT_TREMOLO);
                break;

            default:
                effectStep = 0;
                setPitchShiftEnabled(0);
                setEffect(EFFECT_LOOPBACK);
                break;
        }

        // Atualiza nome do efeito no display OLED com base em effectStep
        oled_show_effect_step_name(effectStep);

        // Feedback visual baseado no efeito atual (LOOPBACK, FLANGER, TREMOLO, REVERB)
        effectChangeFeedback(getCurrentEffect());
    }
    lastEffectButtonState = sw1Raw;
}