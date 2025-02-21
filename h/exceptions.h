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

extern int SYSCALL(int a0, int a1, int a2, int a3);
extern pcb_PTR sysCreateProcess(state_t *statep, support_t *support_p); /* need to add support_t to types.h */
extern void sysTerminate();
extern void sysPasseren(int *semaddr);
extern void sysVerhogen(int *semaddr);
extern void sysWaitIO(int int1no, int dnum, int waitForTermRead);
extern void sysGetCPUTime();
extern void sysWaitClock();
extern void sysGetSupportPTR();

extern void inturruptHandler(); /* to be implemented in inturrupts.c */
extern void programTrapHandler();
extern void TLBExceptionHandler();


/******************************************************************/

#endif 