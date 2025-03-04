#include "../h/exceptions.h"
#include "../h/pcb.h"
#include "../h/asl.h"
#include "../h/const.h"
#include "../h/scheduler.h"
#include "../h/initial.h"
#include "../h/interrupts.h"

/** Handles interrupt exceptions by passing control to the handler
 *  of the highest priority interrupt */
void interruptHandler(){

    /*determine which interrupt is of highest priority */
    int IPline = getHighestPriorityInterrupt();
    
    switch(IPline)
    {
    case 0: /* line 0 can be ignored */
        break;
    case 1: /* line 1 is PLT */
        handlePLTInterrupt();
        break;
    case 2: /* line 2 is interval timer */
        handleIntervalTimerInterrupt();
        break;
    case 3:
    case 4:
    case 5:
    case 6:
    case 7: /* for lines 3-7 there is a bit map for which devices on the line have interrupts */
    handleDeviceInterrupt(IPline);

    }


}

/** handles interrupts from the PLT meaning that the current process's 
 *  Quantum has expired */
void handlePLTInterrupt(){
    /* acknowledge the interrupt by loading the timer with a new value */

    /* copy the processor state at the time of exception into PCBs p_s */
    /* from BIOS data page - make this a deep copy 8?

    /* update CPU time */

    /* place process on the ready Q */

    /* schedule the next process */

}

/** handles interrupts from the pseudo clock, meaning that it is time to
 *  perform a P() operation on the pseudo clock */
void handleIntervalTimerInterrupt(){
    /* acknowledge interrupt by loading interval timer with 100 milliseconds */

    /* unblock ALL PCBs on pseudo clock semaphore */

    /* reset pseudo clock semaphore to 0 */

    /* return control to current process */

}

/** handles the interrupts by the completion of an I/O oppperation */
void handleDeviceInterrupt(int intLine){
    /* calculate the address of this device's device register */
    int highestDevice = getHighestPriorityDevice(intLine);

    /* save off the status code */

    /* acknowledge the inturrup with ACK command */

    /* perform a V() on the semaphore associated with this sub-device */
    /* this should unblock the PCB */
    /* if it returns NULL, return control to current process */

    /* place the saved status code in the v0 of newly unblocked PCB */

    /* insert the PCB on the ready Q */

    /* return control to current process */

}

/** returns the interrupt line of highest priority with a pending interrupt */
int getHighestPriorityInterrupt(){

    /* Get the saved state from the BIOS Data Page */
    state_t *savedState = (state_t *)BIOSDATAPAGE;

    /* interrupt lines with pending interrupts is in cause register */
    /* Extract the interrupt code from the Cause register */
    int IPCode = (savedState->s_cause & IM) >> 8;

    if (IPCode == 0) {
        return -1; /* No set bit found, no inturrupt lines on */
    }

    /* find the lowest set bit in the IP code */    
    int line = 0;
    while ((IPCode & 1) == 0) {
        IPCode >>= 1;
        line++;
    }
    return line;

}

/** returns the device number of highest priority with a pending interrupt */
int getHighestPriorityDevice(int intLine){
    /* the lowest device number with an interrupt is the highest priority */
    int wordNum = intLine - 3;

    int *bitmap = BITMAPADD + (WORDLEN * wordNum);
    int devices = &bitmap;
    devices = (devices & MAPMASK);

    if (devices == 0) {
        return -1; /* No set bit found, no devices inturrupt lines on */
    }

    /* find the lowest set bit in the device map */    
    int line = 0;
    while ((devices & 1) == 0) {
        devices >>= 1;
        line++;
    }
    return line;

}