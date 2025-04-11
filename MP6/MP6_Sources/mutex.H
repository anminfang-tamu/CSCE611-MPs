/*
     File        : mutex.H

     Author      : [YOUR NAME]
     Date        : [CURRENT DATE]

     Description : Simple mutex implementation for thread synchronization

*/

#ifndef _MUTEX_H_
#define _MUTEX_H_

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "thread.H"
#include "machine.H"

/*--------------------------------------------------------------------------*/
/* Mutex */
/*--------------------------------------------------------------------------*/

class Mutex
{
private:
    bool locked;   // Mutex state (true = locked, false = unlocked)
    Thread *owner; // Thread that owns the mutex (if locked)

public:
    Mutex(); // Constructor

    void lock();     // Acquire the mutex (block if already locked)
    void unlock();   // Release the mutex
    bool try_lock(); // Try to acquire the mutex, return true if successful

    bool is_locked() { return locked; }
    Thread *get_owner() { return owner; }
};

#endif