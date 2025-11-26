//////////////////////////////////////////////////////////////////////////////
// * File name: main.c  (VERSÃO CORRIGIDA)
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
#include "effects.h"
#include "csl_chip.h"
#include "stdio.h"

void configPort(void);
void checkTimer(void);
void checkSwitch(void);

extern void initPLL(void);
extern void initAIC3204( );
extern Int16 oled_start( );
extern void VECSTART(void);

extern Uint16 timerFlag;  
Uint8 ledNum = 3;
Uint8 sw1State = 0;
Uint8 sw2State = 0;

// Variável global do efeito
extern volatile Uint8 currentEffect;

void main(void)
{
    //------------------------------------------------------------------
    // 1) CONFIGURAÇÃO CENTRALIZADA DO CSL + VETORES DO DSP
    //------------------------------------------------------------------
    CSL_init();
    IRQ_setVecs((Uint32)(&VECSTART));
    IRQ_globalDisable();
    //------------------------------------------------------------------

    initPLL();         
    EZDSP5502_init();
    initLed();
    configPort();

    initTimer0();      
    initAIC3204();     

    initTremoloEffect();

    configAudioDma();  

    IRQ_globalEnable();   // habilita interrupções após TUDO configurado

    startAudioDma();
    EZDSP5502_MCBSP_init();
    startTimer0();
    oled_start();

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
        CHIP_FSET(ST1_55, XF,CHIP_ST1_55_XF_OFF);
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

        if( ledNum > 6)
            ledNum = 3;
        ledNum++;
    }
}

void checkSwitch(void)
{
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

    if(!(EZDSP5502_I2CGPIO_readLine(SW1)))
    {
        if(sw2State)
        {
            sw2State = 0;
        }
    }
    else
        sw2State = 1;
}
