#include "../h/initial.h"
#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/const.h"

/* Global Variables */
int processCount = 0;                        /* Active process count */
int softBlockCount = 0;                      /* Soft-blocked process count */
pcb_t *readyQueue = NULL;                    /* Tail pointer to ready queue */
pcb_t *currentProcess = NULL;                /* Currently running process */
int deviceSemaphores[NUM_DEVICES + 1] = {0}; /* Device semaphores (extra one for pseudo-clock) */

/**
 * Initializes the Level 3 system.
 * - Sets up the Pass Up Vector
 * - Prepares exception and TLB refill handlers
 */
void initSystem()
{
    /* Get the Pass Up Vector from BIOS Data Page */
    passupvector_t *passupvector = (passupvector_t *)PASSUPVECTOR;

    /* Set the TLB Refill event handler */
    passupvector->tlb_refll_handler = (memaddr)uTLB_RefillHandler;
    passupvector->tlb_refll_stackPtr = (memaddr)0x20001000;

    /* Set the Exception handler */
    passupvector->exception_handler = (memaddr)exceptionHandler;
    passupvector->exception_stackPtr = (memaddr)0x20001000;
}