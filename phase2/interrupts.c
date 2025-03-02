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

    /*determine which interrupt is of highest priority*/



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
    /* interrupt lines with pending interrupts is in cause register */

    /* line number denotes priority (lower number : higher priority) */


    /* for lines 3-7 there is a bit map for which devices on the line have interrupts */

}

/** returns the device number of highest priority with a pending interrupt */
int getHighestPriorityDevice(int intLine){
    /* the lowest device number with an interrupt is the highest priority */

}