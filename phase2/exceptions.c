#include "../h/exceptions.h"
#include "../h/interrupts.h"
#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/const.h"
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
    int exceptionCode = (savedState->s_cause & CAUSEMASK) >> 2;

    switch (exceptionCode)
    {
    case 0: /* External Device Interrupt */
        interruptHandler();
        break;

    case 1:
    case 2:
    case 3: /* TLB Exceptions */
        passUpOrDie(PGFAULTEXCEPT);
        break;

    case 4:
    case 5:
    case 6:
    case 7:
    case 9:
    case 10:
    case 11:
    case 12: /* Program Traps */
        passUpOrDie(GENERALEXCEPT);
        break;

    case 8: /* SYSCALL */
        /* Check if SYSCALL number is 9 or higher */
        if (savedState->s_a0 >= 9)
        {
            passUpOrDie(GENERALEXCEPT);
        }
        else
        {
            syscallHandler(savedState);
        }
        break;

    default:
        /* Undefined exception, terminate the process */
        sysTerminate(currentProcess);
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
        /* Simulate a Program Trap exception */
        savedState->s_cause = (savedState->s_cause & ~CAUSEMASK) | (RESVINSTR << CAUSEINTOFFS);
        programTrapHandler(savedState);
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
        sysWaitIO(savedState, savedState->s_a1, savedState->s_a2, savedState->s_a3);
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
        sysTerminate(currentProcess);
        scheduler();
    }

    /* Move to the next instruction after syscall */
    savedState->s_pc += 4;
    LDST(savedState);
}

/** 
 * Creates a new process with the provided processor state.
 * Inserts the process into the Ready Queue and sets its parent.
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
 * Terminates a process and all its progeny recursively.
 * Ensures proper cleanup of semaphores and process tree.
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

    /* If the process is blocked on a semaphore */
    if (p->p_semAdd != NULL)
    {
        int *semAddr = p->p_semAdd;

        /* Check if it's NOT a device semaphore before adjusting */
        if (!(semAddr >= &deviceSemaphores[0] && semAddr <= &deviceSemaphores[NUM_DEVICES - 1]))
        {
            (*semAddr)++; /* Adjust the semaphore if it's NOT a device semaphore */
        }

        outBlocked(p);

        /* If the process was soft-blocked (waiting for I/O), decrement softBlockCount */
        if (semAddr >= &deviceSemaphores[0] && semAddr <= &deviceSemaphores[NUM_DEVICES - 1])
        {
            softBlockCount--;
        }
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

    /* If no more processes exist, HALT */
    if (processCount == 0)
    {
        HALT();
    }
}

/** 
 * Performs the P operation on the given semaphore. 
 * Blocks the process if necessary and calls the scheduler.
 */
void sysPasseren(int *semaddr)
{
    /* Update CPU time */
    updateCPUTime();

    /* Decrement the semaphore */
    (*semaddr)--;

    /* If semaphore is negative, block the process */
    if (*semaddr < 0)
    {
        /* Save process state */
        memcpy(&(currentProcess->p_s), (state_t *)BIOSDATAPAGE, sizeof(state_t));

        /* Block current process and add it to the semaphore queue */
        currentProcess->p_semAdd = semaddr;
        insertBlocked(semaddr, currentProcess);

        /* Call the scheduler to select the next process */
        scheduler();
    }
}

/** 
 * Performs the V operation on the given semaphore.
 * Unblocks one waiting process, if any.
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

/** 
 * Transitions the current process from running to blocked:
 * performs a P opperation on the semaphore for the IO device.
 * Updates CPU time and state, insertBlocked, and calls scheduler.
 */
void sysWaitIO(state_t *savedState, int intLineNo, int devNum, int waitForTermRead)
{
    int deviceIndex;

    /* Compute the device index */
    if (intLineNo == TERMINT)
    {
        /* Terminals: Separate semaphores for Receiver (0-7) and Transmitter (8-15) */
        deviceIndex = (4 * DEVPERINT) + (devNum * 2) + waitForTermRead;
    }
    else
    {
        /* Other devices: Normal indexing */
        deviceIndex = (intLineNo - 3) * DEVPERINT + devNum;
    }

    int *semaddr = &deviceSemaphores[deviceIndex];

    /* Perform P operation on the device semaphore (blocks if necessary) */
    sysPasseren(semaddr);

    /* Process should be blocked now; once unblocked, store device status */
    savedState->s_v0 = ((device_t *)DEV_REG_ADDR(intLineNo, devNum))->d_status;
}

/** 
 * Retrieves the accumulated CPU time for the current process. 
*/
void sysGetCPUTime(state_t *savedState)
{
    cpu_t currentTOD;
    STCK(currentTOD); /* Store current TOD clock value */

    /* Compute total CPU time: saved time + (current time - last recorded time) */
    savedState->s_v0 = currentProcess->p_time + (currentTOD - currentProcess->p_startTOD);
}

/** 
 * Performs a P opperation on the nucleus maintained pseudoclock
 * semaphore. Blocks the current process on the ASL, calls the
 * scheduler. 
 */
void sysWaitClock()
{
    /* Update CPU time */
    updateCPUTime();

    /* Perform P() operation on the pseudo-clock semaphore */
    sysPasseren(&deviceSemaphores[NUM_DEVICES]);
}

/**
 * Returns the support structure pointer of the current process.
 */
void *sysGetSupportPTR()
{
    return currentProcess->p_supportStruct;
}

/**
 * Handles program traps, terminating the offending process.
*/
void programTrapHandler(state_t *savedState)
{
    passUpOrDie(GENERALEXCEPT);
}

/**
 * Handles TLB exceptions.
 */
void TLBExceptionHandler()
{
    passUpOrDie(PGFAULTEXCEPT);
}

/**
 * Updates the CPU time for the current process.
 */
void updateCPUTime()
{
    unsigned int currentTOD; /* TOD clock value */
    STCK(currentTOD);        /* Read the current TOD clock value */

    /* Update the accumulated CPU time */
    currentProcess->p_time += (currentTOD - currentProcess->p_startTOD);

    /* Reset the start time for the next time slice */
    currentProcess->p_startTOD = currentTOD;
}

/** Handles Pass Up or Die mechanism for TLB exceptions, program traps, and SYSCALLs >= 9.
 *  If the current process has a support structure, it passes the exception to the support level.
 *  Otherwise, the process and all its progeny are terminated.
 */
void passUpOrDie(int exceptType)
{
    /* Check if the current process has a support structure */
    if (currentProcess->p_supportStruct == NULL)
    {
        /* No support structure, terminate the process and its children */
        sysTerminate(currentProcess);
        scheduler(); /* Call scheduler after termination */
    }

    /* Copy the saved exception state to the appropriate support structure field */
    currentProcess->p_supportStruct->sup_exceptState[exceptType] = *(state_t *)BIOSDATAPAGE;

    /* Load the exception handler's context */
    LDCXT(currentProcess->p_supportStruct->sup_exceptContext[exceptType].c_stackPtr,
          currentProcess->p_supportStruct->sup_exceptContext[exceptType].c_status,
          currentProcess->p_supportStruct->sup_exceptContext[exceptType].c_pc);
}
