//////////////////////////////////////////////////////////////////////////////
// * File name: pll.c
// *                                                                          
// * Description:  PLL functions.
// *                                                                          
// * Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/ 
// * Copyright (C) 2011 Spectrum Digital, Incorporated
// *                                                                          
// *                                                                          
// *  Redistribution and use in source and binary forms, with or without      
// *  modification, are permitted provided that the following conditions      
// *  are met:                                                                
// *                                                                          
// *    Redistributions of source code must retain the above copyright        
// *    notice, this list of conditions and the following disclaimer.         
// *                                                                          
// *    Redistributions in binary form must reproduce the above copyright     
// *    notice, this list of conditions and the following disclaimer in the   
// *    documentation and/or other materials provided with the                
// *    distribution.                                                         
// *                                                                          
// *    Neither the name of Texas Instruments Incorporated nor the names of   
// *    its contributors may be used to endorse or promote products derived   
// *    from this software without specific prior written permission.         
// *                                                                          
// *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS     
// *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT       
// *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR   
// *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT    
// *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,   
// *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT        
// *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,   
// *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY   
// *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT     
// *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE   
// *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.    
// *                                                                          
//////////////////////////////////////////////////////////////////////////////
#include "ezdsp5502.h"
#include "csl.h"
#include "csl_pll.h"

/* PLL configuration structure */
PLL_Config myCfg =
{
    PLL_PLLCSR_RMK(
        PLL_PLLCSR_PLLRST_RESET_RELEASED,
        PLL_PLLCSR_OSCPWRDN_OSC_ON,
        PLL_PLLCSR_PLLPWRDN_PLL_ON,
        PLL_PLLCSR_PLLEN_DEFAULT
    ),
    PLL_PLLM_PLLM_OF(10),
    PLL_PLLDIV0_RMK(
        PLL_PLLDIV0_D0EN_ENABLED,
        PLL_PLLDIV0_PLLDIV0_OF(0)
    ),
    PLL_PLLDIV1_RMK(
        PLL_PLLDIV1_D1EN_ENABLED,
        PLL_PLLDIV1_PLLDIV1_OF(0)
    ),
    PLL_PLLDIV2_RMK(
        PLL_PLLDIV2_D2EN_ENABLED,
        PLL_PLLDIV2_PLLDIV2_OF(0)
    ),
    PLL_PLLDIV3_RMK(
        PLL_PLLDIV3_D3EN_ENABLED,
        PLL_PLLDIV3_PLLDIV3_OF(0)
    ),
    PLL_OSCDIV1_RMK(
        PLL_OSCDIV1_OD1EN_DISABLED,
        PLL_OSCDIV1_OSCDIV1_OF(0)
    ),
    PLL_WKEN_RMK(
        PLL_WKEN_WKEN4_ENABLED,
        PLL_WKEN_WKEN3_ENABLED,
        PLL_WKEN_WKEN2_ENABLED,
        PLL_WKEN_WKEN1_ENABLED,
        PLL_WKEN_WKEN0_ENABLED
    ),
    PLL_CLKMD_RMK(
        PLL_CLKMD_CLKMD0_OSCOUT
    ),
    PLL_CLKOUTSR_RMK(
        PLL_CLKOUTSR_CLKOSEL_SYSCLK1,
        PLL_CLKOUTSR_CLKOUTDIS_ENABLED
    )
};

/*
 *  initPLL( )
 *
 *    Initialize PLL with some initial values then set it to
 *      work at 300MHz
 */
void initPLL(void)
{
    PLL_config(&myCfg); 
    
    /*Parameters for PLL_setFreq are:   
    enable/mode, multiply, divider0, divider1, divider2, divider3, osc divider
    
        mode  = 1 means PLL enabled (non-bypass mode)
        mul   = 15 means multiply by 15
        div0  = 0 means divide by 1 using PLLDIV0
        div1  = 3 means divide by 4 using PLLDIV1
        div2  = 3 means divide by 4 using PLLDIV2
        div3  = 3 means divide by 4 using PLLDIV3
        oscdiv= 1 means divide by 2 using OSCDIV1
    */
    
    /* Set the required frequncy for CPU, Fast and Slow peripherals and EMIF */
    PLL_setFreq(1,15,0,3,3,3,1); // CPU @ 300 MHz, others @ 75MHz
    
}
