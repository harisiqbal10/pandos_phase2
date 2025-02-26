#include "../h/exceptions.h"
#include "../h/initial.h"
#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/const.h"


/** checks the excCode in the cause register to pass on the execption
 *  to the correct handler */
void exceptionHandler(){


    /* 0 is inturrupts */

    /* 1-3 is TLB */

    /* 8 is SYSCALL */

    /* 4-7 and 9-12 (everything else) are program traps */


}


/** checks the value in a0 to determine which syscall
 *  was made and passes along to the specified syscall 
 *  function */
void syscallHandler(){

    /* 1 is createProcess */

    /* 2 is terminateProcess */

    /* 3 is passeren */

    /* 4 is verhogen */

    /* 5 is waitIO */

    /* 6 is getCPUTime */

    /* 7 is waitClock */

    /* 8 is getSupportPTR */



}


/** creates a process with the processor state in a1
 *  if a new process cannot be created an error code is 
 *  returned in v0 */
void sysCreateProcess(){

}

/** terminates the executing process */
void sysTerminate(){

}

/** performs the P opperation (wait) on the semaphore
 *  with the address stored in a1 */
void sysPasseren(){

}


/** performs the V opperation (signal) on the semaphore
 *  with the address stored in a1 */
void sysVerhogen(){

}

/** transitions the current process from running to blocked:
 *  performs a P opperation on the semaphore for the IO device
 *  updates CPU time and state, insertBlocked, and calls scheduler */
void sysWaitIO(){

}

/** accumulated processor time of a process is placed in the callers v0 */
void sysGetCPUTime(){

}

/** performs a P opperation on the nucleus maintained pseudoclock 
 *  semaphore. Blocks the current process on the ASL, calls the 
 *  scheduler. */
void sysWaitClock(){

}

/** gets a pointer to the current process's support structure
 *  if no value for p_supportStruct was provided when the 
 *  current process was created, return NULL */
void sysGetSupportPTR(){

}

