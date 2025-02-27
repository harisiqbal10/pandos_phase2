#ifndef EXCEPTIONS
#define EXCEPTIONS

/************************* EXCEPTIONS.H *****************************
 *
 *  The externals declaration file for the Exceptions
 *  module.
 *
 *  Implements an exception handler, SYSCALL handler, SYSCALLs 1-8,
 *  program trap exception handler, and TLB exception handler
 *
 */

#include "../h/types.h"
#include "../h/const.h"
#include "../h/pcb.h"
#include "../h/asl.h"

extern void exceptionHandler();

extern void syscallHandler();
extern int  sysCreateProcess(); 
extern void sysTerminate();
extern void sysPasseren();
extern void sysVerhogen();
extern void sysWaitIO();
extern void sysGetCPUTime();
extern void sysWaitClock();
extern void *sysGetSupportPTR();

extern void inturruptHandler(); /* to be implemented in inturrupts.c */
extern void programTrapHandler();
extern void TLBExceptionHandler();


/******************************************************************/

#endif 