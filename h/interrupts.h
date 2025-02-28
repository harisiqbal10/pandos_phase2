#ifndef INTERRUPTS
#define INTERRUPTS

/************************* INTERRUPTS.H *****************************
 *
 *  The externals declaration file for the Interrupts
 *  module.
 *
 *  Implements an interrupt handler that determines the highest priority
 *  pending interrupt and dispatches it to the appropriate handler.
 *
*/

#include "../h/types.h"


void interruptHandler();
void handlePLTInterrupt();
void handleIntervalTimerInterrupt();
void handleDeviceInterrupt(int intLine);
int getHighestPriorityInterrupt();
int getHighestPriorityDevice(int intLine);

/*******************************************************************/

#endif 
