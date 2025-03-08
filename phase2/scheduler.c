#include "../h/scheduler.h"
#include "../h/initial.h"
#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/const.h"


void debug(int a, void* b){
    int i=1;
    int j=2;
    i= i + j;
    return;
}

/**
 * The scheduler selects the next process to run and dispatches it.
 * If no process is ready, it handles termination, waiting, or deadlock scenarios.
 */
void scheduler()
{   
    
    /* Select the next process to run */
    currentProcess = removeProcQ(&readyQueue);

    debug(10, 0);
    void * temp = ((void *)0xFFFFFFFF);
    debug((int)currentProcess, temp);

    /* If no ready process exists, handle special cases */
    if (currentProcess == temp)
    {

        debug( processCount, temp);

        if (processCount == 0)
        {
            HALT(); /* No active processes, system halts */
        }
        else if (softBlockCount > 0)
        {
            /* Wait for an I/O or timer interrupt */
            debug(10, temp);
            setSTATUS(getSTATUS() | IEPBITON | KUPBITON | IM); /* Enable interrupts */
            WAIT();
        }
        else
        {
            PANIC(); /* Deadlock detected */
        }
    }

    /* Load the Process Local Timer (PLT) with 5 milliseconds */
    setTIMER(5000);

    /* Load the process state and execute */
    LDST(&(currentProcess->p_s));
}
