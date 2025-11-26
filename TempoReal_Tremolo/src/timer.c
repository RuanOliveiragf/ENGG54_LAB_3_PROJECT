//////////////////////////////////////////////////////////////////////////////
// * File name: timer.c
//////////////////////////////////////////////////////////////////////////////
#include "ezdsp5502.h"
#include "csl.h"  
#include "csl_gpt.h"
#include "csl_irq.h"
#include "timer.h"
#include "isr.h"

Uint8 timerState = 0;  // Indicates Timer's period state
Uint16 timerFlag = 0;  // Flag indicating a timer interrupt
Uint16 gpt12Evt_Id;

GPT_Handle myhGpt;

/* Timer configuration structure */
GPT_Config myGptCfg = 
{
    0,
    GPT_GPTGPINT_RMK(
        GPT_GPTGPINT_TIN1INV_DEFAULT,
        GPT_GPTGPINT_TIN1INT_DEFAULT
    ),
    GPT_GPTGPEN_RMK(
        GPT_GPTGPEN_TOUT1EN_DEFAULT,
        GPT_GPTGPEN_TIN1EN_DEFAULT  
    ),
    GPT_GPTGPDIR_RMK(
        GPT_GPTGPDIR_TOUT1DIR_DEFAULT,
        GPT_GPTGPDIR_TIN1DIR_DEFAULT
    ),
    GPT_GPTGPDAT_RMK(
        GPT_GPTGPDAT_TOUT1DAT_DEFAULT,
        GPT_GPTGPDAT_TIN1DAT_DEFAULT
    ),
    0x68C0, //PRD1
    0x0478, //PRD2
    0xFFFF, //PRD3
    0xFFFF, //PRD4
    GPT_GPTCTL1_RMK(
        GPT_GPTCTL1_TIEN_DEFAULT,
        GPT_GPTCTL1_CLKSRC_DEFAULT,
        GPT_GPTCTL1_ENAMODE_CONTINUOUS,
        GPT_GPTCTL1_PWID_DEFAULT,
        GPT_GPTCTL1_CP_DEFAULT,
        GPT_GPTCTL1_INVIN_DEFAULT,
        GPT_GPTCTL1_INVOUT_DEFAULT          
    ),
    GPT_GPTCTL2_RMK(
        GPT_GPTCTL2_TIEN_DEFAULT,
        GPT_GPTCTL2_CLKSRC_DEFAULT,
        GPT_GPTCTL2_ENAMODE_ONCE,
        GPT_GPTCTL2_PWID_DEFAULT,
        GPT_GPTCTL2_CP_DEFAULT,
        GPT_GPTCTL2_INVIN_DEFAULT,
        GPT_GPTCTL2_INVOUT_DEFAULT
    ),
    GPT_GPTGCTL1_RMK(
        GPT_GPTGCTL1_TDDR34_DEFAULT,
        GPT_GPTGCTL1_PSC34_DEFAULT,
        GPT_GPTGCTL1_TIMMODE_32BIT_DUAL,
        GPT_GPTGCTL1_TIM34RS_IN_RESET,
        GPT_GPTGCTL1_TIM12RS_IN_RESET
    )       
};

/*
 *  initTimer0( )
 *
 *    Configure Timer and setup timer interrupt
 */
void initTimer0(void)
{
    /* Open GPT0 */
    myhGpt = GPT_open(GPT_DEV0, GPT_OPEN_RESET);
    
    gpt12Evt_Id = GPT_getEventId(myhGpt);  // Get GPTx EventId
    IRQ_clear(gpt12Evt_Id);                // Clear interrupt
    IRQ_plug(gpt12Evt_Id, &gpt0Isr);       // Add ISR to vector table
    IRQ_enable(gpt12Evt_Id);               // Enable timer interrupt

    GPT_config(myhGpt, &myGptCfg);         // Configure timer with 1 sec period
}

/*
 *  startTimer0( )
 *
 *    Start Timer0
 */
void startTimer0(void)
{
    GPT_start12(myhGpt);
}

/*
 *  gpt0Isr( )
 *
 *    Timer0 Interrupt Service Routine
 */
interrupt void gpt0Isr(void)
{
    timerFlag = 1;
}

/*
 *  changeTimer( )
 *
 *    Swap between a 1 sec period and a 250ms period
 */
void changeTimer(void)
{
    if(timerState == 0)
    {
        GPT_stop12(myhGpt);                 // Stop Timer0
        GPT_RSETH(myhGpt,GPTPRD1, 0x1A30);  //
        GPT_RSETH(myhGpt,GPTPRD2, 0x011E);  // Reset period to 250ms
        GPT_RSETH(myhGpt,GPTCNT1, 0x0000);  //
        GPT_RSETH(myhGpt,GPTCNT2, 0x0000);  // Reset counter to 0
        GPT_start12(myhGpt);                // Start Timer
        timerState = 1;                     // Change state
    }
    else
    {
        GPT_stop12(myhGpt);                 // Stop Timer0
        GPT_RSETH(myhGpt,GPTPRD1, 0x68C0);  //
        GPT_RSETH(myhGpt,GPTPRD2, 0x0478);  // Reset period to 1 sec
        GPT_RSETH(myhGpt,GPTCNT1, 0x0000);  //
        GPT_RSETH(myhGpt,GPTCNT2, 0x0000);  // Reset counter to 0
        GPT_start12(myhGpt);                // Start Timer
        timerState = 0;                     // Change state
    }
}
