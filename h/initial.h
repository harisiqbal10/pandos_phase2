#ifndef INITIAL
#define INITIAL

/************************* INITIAL.H *****************************
 *
 *  The externals declaration file for the system initialization
 *  module.
 *
 *  Implements system initialization, process creation, and global
 *  variables required by the nucleus.
 *
 */

#include "../h/types.h"
#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/const.h"

#define NUM_DEVICES (DEVINTNUM * DEVPERINT) /* 40 total device semaphores */

/* Global Variables */
extern int processCount;                  
extern int softBlockCount;                
extern pcb_PTR readyQueue;               
extern pcb_PTR currentProcess;            
extern int deviceSemaphores[NUM_DEVICES]; 

/* Function Prototypes */
extern void initNucleus();
extern void createProcess();
extern void uTLB_RefillHandler(); /* Provided placeholder function */
extern void exceptionHandler();   /* To be implemented in exceptions.c */

/***************************************************************/

#endif
