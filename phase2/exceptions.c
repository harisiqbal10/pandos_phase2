#include "../h/exceptions.h"
#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/const.h"
#include "../h/types.h"
#include "../h/scheduler.h"
#include "../h/initial.h"

/**
 * Handles exceptions that occur during execution.
 * The exception type is determined by examining the Cause register.
 */
void exceptionHandler()
{
    /* Get the saved state from the BIOS Data Page */
    state_t *savedState = (state_t *)BIOSDATAPAGE;

    /* Extract the exception code from the Cause register */
    int exceptionCode = (savedState->s_cause & 0x0000007C) >> 2;

    switch (exceptionCode)
    {
    case 0: /* External Device Interrupt */
        interruptHandler();
        break;
    case 1:
    case 2:
    case 3: /* TLB Exceptions */
        TLBExceptionHandler();
        break;
    case 4:
    case 5:
    case 6:
    case 7:
    case 9:
    case 10:
    case 11:
    case 12: /* Program Traps */
        programTrapHandler(savedState);
        break;
    case 8: /* SYSCALL */
        syscallHandler(savedState);
        break;
    default:
        /* Undefined exception, terminate the process */
        terminateProcess(currentProcess);
        scheduler();
    }
}

/**
 * Handles SYSCALL exceptions.
 * The function number is stored in register a0 of the saved state.
 * If a syscall is invoked from user mode, the process is terminated.
 */
void syscallHandler(state_t *savedState)
{
    /* Check if the process is in user mode */
    if ((savedState->s_status & KUPBITON) != 0)
    {
        /* If in user mode, terminate process */
        terminateProcess(currentProcess);
        scheduler();
    }

    /* Retrieve syscall number from register a0 */
    int syscallNumber = savedState->s_a0;

    switch (syscallNumber)
    {
    case CREATEPROCESS:
        /* Process creation */
        savedState->s_v0 = sysCreateProcess((state_t *)savedState->s_a1, (support_t *)savedState->s_a2);
        break;
    case TERMINATEPROCESS:
        sysTerminate(currentProcess);
        scheduler();
        break;
    case PASSEREN:
        /* Perform P() operation on a semaphore */
        sysPasseren((int *)savedState->s_a1);
        break;
    case VERHOGEN:
        /* Perform V() operation on a semaphore */
        sysVerhogen((int *)savedState->s_a1);
        break;
    case WAITIO:
        /* Block process waiting for I/O */
        sysWaitIO(savedState->s_a1, savedState->s_a2, savedState->s_a3);
        break;
    case GETCPUTIME:
        /* Return accumulated CPU time */
        sysGetCPUTime(savedState);
        break;
    case WAITCLOCK:
        /* Block process until next pseudo-clock tick */
        sysWaitClock();
        break;
    case GETSUPPORTPTR:
        /* Return the process's support structure */
        savedState->s_v0 = (int)sysGetSupportPTR();
        break;
    default:
        /* Invalid syscall, terminate the process */
        terminateProcess(currentProcess);
        scheduler();
    }

    /* Move to the next instruction after syscall */
    savedState->s_pc += 4;
    savedState->s_t9 += 4;
    LDST(savedState);
}

/** creates a process with the processor state in a1
 *  if a new process cannot be created an error code is
 *  returned in v0 
 */
int sysCreateProcess(state_t *statep, support_t *supportp)
{
    /* Allocate a new PCB */
    pcb_t *newProcess = allocPcb();
    if (newProcess == NULL)
    {
        return -1; /* No more free pcbs, return error */
    }

    /* Initialize process state from provided state pointer */
    newProcess->p_s = *statep;
    newProcess->p_supportStruct = supportp; /* Set support structure (NULL if not provided) */

    /* Initialize other process fields */
    newProcess->p_time = 0;      /* Reset CPU time */
    newProcess->p_semAdd = NULL; /* Not blocked on any semaphore */

    /* Make it a child of the current process */
    insertChild(currentProcess, newProcess);

    /* Insert into Ready Queue */
    insertProcQ(&readyQueue, newProcess);
    processCount++;

    return 0; /* Success */
}

/**
 * Recursively terminates a process and all its progeny.
 * - Removes the process from the process tree.
 * - Recursively calls itself on child processes.
 * - Removes the process from any queues.
 * - If the process is blocked on a semaphore, it adjusts the semaphore count.
 * - Frees the PCB.
 */
void sysTerminate(pcb_t *p)
{
    if (p == NULL)
        return;

    /* Recursively terminate all children */
    while (!emptyChild(p))
    {
        sysTerminate(removeChild(p));
    }

    /* If the process is blocked on a semaphore, remove it */
    if (p->p_semAdd != NULL)
    {
        outBlocked(p);
    }

    /* Remove process from the Ready Queue if it is in it */
    outProcQ(&readyQueue, p);

    /* If the process has a parent, detach it */
    if (p->p_prnt != NULL)
    {
        outChild(p);
    }

    /* Free the PCB */
    freePcb(p);

    /* Decrease active process count */
    processCount--;
}

/** performs the P opperation (wait) on the semaphore
 *  with the address stored in a1 
 */
void sysPasseren(int *semaddr)
{
    /* Decrement the semaphore */
    (*semaddr)--;

    /* If semaphore is negative, block the process */
    if (*semaddr < 0)
    {
        /* Block current process and add it to the semaphore queue */
        currentProcess->p_semAdd = semaddr;
        insertBlocked(semaddr, currentProcess);

        /* Call the scheduler to select the next process */
        scheduler();
    }
}

/** performs the V opperation (signal) on the semaphore
 *  with the address stored in a1 
 */
void sysVerhogen(int *semAddr)
{
    /* Increment the semaphore value */
    (*semAddr)++;

    /* If any process is blocked on this semaphore, unblock the first one */
    pcb_t *unblockedProcess = removeBlocked(semAddr);

    if (unblockedProcess != NULL)
    {
        insertProcQ(&readyQueue, unblockedProcess); /* Move to Ready Queue */
    }
}

/** transitions the current process from running to blocked:
 *  performs a P opperation on the semaphore for the IO device
 *  updates CPU time and state, insertBlocked, and calls scheduler 
 */
void sysWaitIO(int intLineNo, int devNum, int waitForTermRead)
{
    int deviceIndex;

    /* Compute the device index */
    if (intLineNo == TERMINT)
    {
        /* Terminals: Separate semaphores for Receive (0-7) and Transmit (8-15) */
        deviceIndex = (4 * DEVPERINT) + (devNum * 2) + waitForTermRead;
    }
    else
    {
        /* Other devices: Normal indexing */
        deviceIndex = (intLineNo - 3) * DEVPERINT + devNum;
    }

    /* Perform P operation on the device semaphore */
    passeren(&deviceSemaphores[deviceIndex]);

    /* Store the device's status register value in v0 */
    state_t *savedState = (state_t *)BIOSDATAPAGE;
    savedState->s_v0 = ((device_t *)DEV_REG_ADDR(intLineNo, devNum))->d_status;
}

/** accumulated processor time of a process is placed in the callers v0 */
void sysGetCPUTime(state_t *savedState)
{
    /* Store the accumulated CPU time of the current process in v0 */
    savedState->s_v0 = currentProcess->p_time;
}

/** performs a P opperation on the nucleus maintained pseudoclock
 *  semaphore. Blocks the current process on the ASL, calls the
 *  scheduler. 
 */
void sysWaitClock()
{
    /* Perform P() operation on the pseudo-clock semaphore */
    passeren(&deviceSemaphores[NUM_DEVICES]);

    /* After blocking, call the scheduler */
    scheduler();
}

/** gets a pointer to the current process's support structure
 *  if no value for p_supportStruct was provided when the
 *  current process was created, return NULL 
 */
void *sysGetSupportPTR()
{
    /* Return the support structure pointer of the Current Process */
    return currentProcess->p_supportStruct;
}

/**
 * Handles program trap exceptions.
 * These include bus errors, reserved instruction errors, and overflow exceptions.
 * Any process causing a trap is terminated.
 */
void programTrapHandler(state_t *savedState)
{
    terminateProcess(currentProcess);
    scheduler();
}

/**
 * Handles TLB exceptions.
 * This is a placeholder function since TLB handling is typically implemented in later phases.
 */
void TLBExceptionHandler()
{
    PANIC(); /* Not implemented in this phase */
}

/**
 * Terminates the given process and removes it from the system.
 * If the process has children, they are also recursively terminated.
 */
void terminateProcess(pcb_t *p)
{
    if (p == NULL)
        return;

    /* Recursively terminate child processes */
    while (!emptyChild(p))
    {
        terminateProcess(removeChild(p));
    }

    /* Remove process from any queues */
    if (p->p_semAdd != NULL)
    {
        outBlocked(p);
    }
    else
    {
        outProcQ(&readyQueue, p);
    }

    freePcb(p);
    processCount--;

    /* If no more processes, halt the system */
    if (processCount == 0)
    {
        HALT();
    }
}
