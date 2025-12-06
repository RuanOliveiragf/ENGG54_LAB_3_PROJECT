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
extern void VECSTART(void);

// Protótipos
void configPort(void);
void checkTimer(void);
void checkSwitch(void);
void effectChangeFeedback(Uint8 effect);

// Variáveis globais
extern Uint16 timerFlag;  
Uint8 ledNum = 3;
Uint8 sw1State = 0;
Uint8 sw2State = 0;

void main(void)
{
    // 1. Configuração centralizada do CSL
    CSL_init();
    IRQ_setVecs((Uint32)(&VECSTART));
    IRQ_globalDisable();
    
    // 2. Inicializações básicas
    initPLL();         
    EZDSP5502_init();
    initLed();
    configPort();
    initTimer0();      
    initAIC3204();     
    
    // 3. Inicializa sistema de efeitos
    initEffectController();
    
    // 4. Configura DMA
    configAudioDma();  
    
    // 5. Habilita interrupções
    IRQ_globalEnable();
    
    // 6. Inicia periféricos
    startAudioDma();
    EZDSP5502_MCBSP_init();
    startTimer0();
    oled_start();
    
    // 7. Loop principal
    while(1)
    {
        checkTimer();
        checkSwitch();
    }
}

void configPort(void)
{
    EZDSP5502_I2CGPIO_configLine( BSP_SEL1, OUT );
    EZDSP5502_I2CGPIO_writeLine(  BSP_SEL1, LOW );
    EZDSP5502_I2CGPIO_configLine( BSP_SEL1_ENn, OUT );
    EZDSP5502_I2CGPIO_writeLine(  BSP_SEL1_ENn, LOW );
}

static void toggleLED(void)
{
    if(CHIP_FGET(ST1_55, XF))
        CHIP_FSET(ST1_55, XF, CHIP_ST1_55_XF_OFF);
    else
        CHIP_FSET(ST1_55, XF, CHIP_ST1_55_XF_ON);
}

void checkTimer(void)
{
    if(timerFlag == 1)
    {
        timerFlag = 0;
        toggleLED();
        EZDSP5502_I2CGPIO_writeLine( ledNum,
           (~EZDSP5502_I2CGPIO_readLine(ledNum) & 0x01) );
        if( ledNum > 6) ledNum = 3;
        ledNum++;
    }
}

// Feedback visual para mudança de efeito
void effectChangeFeedback(Uint8 effect)
{
    // Pisca LED correspondente ao efeito
    Uint8 led = effect % 4;
    
    // Acende o LED
    EZDSP5502_I2CGPIO_writeLine(LED0 + led, LOW);
    
    // Aguarda
    volatile int i;
    for(i = 0; i < 50000; i++);
    
    // Apaga o LED
    EZDSP5502_I2CGPIO_writeLine(LED0 + led, HIGH);
}

void checkSwitch(void)
{
    static Uint8 lastButtonState = 1;
    static Uint8 debounceCounter = 0;
    
    /* Check SW1 - Controla timer */
    if(!(EZDSP5502_I2CGPIO_readLine(SW0)))
    {
        if(sw1State)
        {
            changeTimer();
            sw1State = 0;
        }
    }
    else
        sw1State = 1;

    /* Check SW2 - Alterna entre efeitos */
    Uint8 currentButtonState = EZDSP5502_I2CGPIO_readLine(SW1);
    
    // Debouncing
    if (currentButtonState == 0 && lastButtonState == 1) {
        debounceCounter++;
        if (debounceCounter > 5) {  // ~50ms @ 100Hz checking
            // Botão pressionado confirmado
            
            // Obtém próximo efeito
            Uint8 nextEffect = getNextEffect(getCurrentEffect());
            
            // Aplica o efeito (com limpeza automática do anterior)
            setEffect(nextEffect);
            
            // Feedback visual
            effectChangeFeedback(nextEffect);
            
            debounceCounter = 0;
        }
    } else {
        debounceCounter = 0;
    }
    
    lastButtonState = currentButtonState;
}
