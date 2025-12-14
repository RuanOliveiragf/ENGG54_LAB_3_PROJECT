*****************************************************************************
* File name: vectors.asm
*                                                                          
* Description:  Assembly file to set up interrupt vector table.
*                                                                          
* Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/ 
* Copyright (C) 2011 Spectrum Digital, Incorporated
*****************************************************************************

        .sect "vectors"

*------------------------------------------------------------------------------
* Global symbols defined here and exported out of this file
*------------------------------------------------------------------------------
        .global _VECSTART
        .global _vecs

*------------------------------------------------------------------------------
* Global symbols referenced in this file but defined somewhere else. 
* Remember that your interrupt service routines need to be referenced here.
*------------------------------------------------------------------------------
        .ref _c_int00
        .ref _dmaRxIsr
        .ref _dmaTxIsr  
        .ref _gpt0Isr

*------------------------------------------------------------------------------
* Interrupt Vector Table for C5502
*------------------------------------------------------------------------------

_VECSTART:
_vecs:  
        .ivec   _c_int00,use_reta      ; Reset

NMI:    .ivec   no_isr                 ; Non-maskable interrupt

INT0:   .ivec   no_isr                 ; External interrupt 0

INT2:   .ivec   no_isr                 ; External interrupt 2

TINT0:  .ivec   _gpt0Isr               ; Timer 0 interrupt (GPT0)

RINT0:  .ivec   no_isr                 ; McBSP0 receive interrupt

RINT1:  .ivec   _dmaRxIsr              ; McBSP1 receive interrupt (DMA RX)

XINT1:  .ivec   _dmaTxIsr              ; McBSP1 transmit interrupt (DMA TX)

LCKINT: .ivec   no_isr                 ; Clock lost interrupt

DMAC1:  .ivec   no_isr                 ; DMA channel 1 interrupt

DSPINT: .ivec   no_isr                 ; Host port interface interrupt

INT3:   .ivec   no_isr                 ; External interrupt 3

UART:   .ivec   no_isr                 ; UART interrupt

XINT2:  .ivec   no_isr                 ; McBSP2 transmit interrupt

DMAC4:  .ivec   no_isr                 ; DMA channel 4 interrupt

DMAC5:  .ivec   no_isr                 ; DMA channel 5 interrupt

INT1:   .ivec   no_isr                 ; External interrupt 1

XINT0:  .ivec   no_isr                 ; McBSP0 transmit interrupt

DMAC0:  .ivec   no_isr                 ; DMA channel 0 interrupt

INT4:   .ivec   no_isr                 ; External interrupt 4

DMAC2:  .ivec   no_isr                 ; DMA channel 2 interrupt

DMAC3:  .ivec   no_isr                 ; DMA channel 3 interrupt

TINT1:  .ivec   no_isr                 ; Timer 1 interrupt

INT5:   .ivec   no_isr                 ; External interrupt 5

*------------------------------------------------------------------------------
* Dummy ISR for unhandled interrupts
*------------------------------------------------------------------------------
        .text
        .def no_isr
no_isr:
        b #no_isr

*------------------------------------------------------------------------------