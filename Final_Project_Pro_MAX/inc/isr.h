//////////////////////////////////////////////////////////////////////////////
// isr.h - Header file for interrupt service routines
//////////////////////////////////////////////////////////////////////////////

#ifndef ISR_H_
#define ISR_H_

#include "ezdsp5502.h"

// DMA ISRs
interrupt void dmaRxIsr(void);
interrupt void dmaTxIsr(void);

// Timer ISR
interrupt void gpt0Isr(void);

#endif /* ISR_H_ */