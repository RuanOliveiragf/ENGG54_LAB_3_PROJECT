//////////////////////////////////////////////////////////////////////////////
// timer.h - Header file for timer functions
//////////////////////////////////////////////////////////////////////////////

#ifndef TIMER_H_
#define TIMER_H_

#include "ezdsp5502.h"

// Function prototypes
void initTimer0(void);
void startTimer0(void);
void changeTimer(void);

// External variables  
extern Uint16 timerFlag;

#endif /* TIMER_H_ */