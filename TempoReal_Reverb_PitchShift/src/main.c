//////////////////////////////////////////////////////////////////////////////
// * File name: main.c
// * Descrição: Inicializa Reverb e Pitch Shift em cadeia.
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
#include "csl_chip.h"
#include "stdio.h"
#include "effects.h"
#include "pitch_shift.h"

void configPort(void);
void checkTimer(void);

extern void initPLL(void);
extern void initAIC3204();
extern Int16 oled_start();
extern void VECSTART(void);

extern Uint16 timerFlag;
Uint8 ledNum = 3;

// --- DEFINIÇÃO DA FREQUÊNCIA DE PITCH ---
// Altere este valor para testar diferentes shifts (Ex: 329.63 = Mi/E4)
#define TARGET_FREQ_HZ 220.0f

void main(void)
{
    // 1) Configuração Base do DSP
    CSL_init();
    IRQ_setVecs((Uint32)(&VECSTART));
    IRQ_globalDisable();

    initPLL();
    EZDSP5502_init();
    initLed();
    configPort();

    initTimer0();
    initAIC3204();

    // 2) Inicialização dos Efeitos (Alocação de Memória e Zeramento)
    // Inicializa o Reverb (parâmetros definidos dentro de effects.c/initReverbEffect)
    initReverbEffect();

    // Inicializa o Pitch Shift (Memória e ponteiros)
    initPitchShift();

    // 3) Definição Dinâmica dos Parâmetros
    // Define a frequência alvo do Pitch Shift.
    // Isso pode ser mudado a qualquer momento dentro de um loop se desejar.
    setPitchFrequency(TARGET_FREQ_HZ);

    // 4) Configuração do DMA de Áudio
    configAudioDma();

    IRQ_globalEnable(); // Habilita interrupções

    startAudioDma();
    EZDSP5502_MCBSP_init();
    startTimer0();
    oled_start();

    // Loop Principal
    while(1)
    {
        checkTimer();
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
        EZDSP5502_I2CGPIO_writeLine( ledNum, (~EZDSP5502_I2CGPIO_readLine(ledNum) & 0x01) );
        if( ledNum > 6) ledNum = 3;
        ledNum++;
    }
}
