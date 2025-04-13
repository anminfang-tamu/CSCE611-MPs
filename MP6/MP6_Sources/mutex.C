/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "mutex.H"
#include "thread.H"
#include "system.H"
#include "console.H"
#include "machine.H"

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   M u t e x  */
/*--------------------------------------------------------------------------*/

Mutex::Mutex()
    : locked(false), owner(nullptr)
{
    // Initialize an unlocked mutex
}

void Mutex::lock()
{
    // Disable interrupts while checking and updating mutex state
    bool interrupts_were_enabled = Machine::interrupts_enabled();
    if (interrupts_were_enabled)
    {
        Machine::disable_interrupts();
    }

    // If mutex is already locked, current thread needs to wait
    while (locked)
    {
        // Get the current thread
        Thread *current = Thread::CurrentThread();

        // We need to yield to another thread
        // Re-enable interrupts before yielding if they were enabled
        if (interrupts_were_enabled)
        {
            Machine::enable_interrupts();
        }

        // Yield to other threads and try again later
        System::SCHEDULER->resume(current);
        System::SCHEDULER->yield();

        // When we come back from yield, disable interrupts again if they were enabled
        if (interrupts_were_enabled && Machine::interrupts_enabled())
        {
            Machine::disable_interrupts();
        }
    }

    // Now the mutex is available, lock it
    locked = true;
    owner = Thread::CurrentThread();

    // Re-enable interrupts if they were enabled before
    if (interrupts_were_enabled)
    {
        Machine::enable_interrupts();
    }
}

void Mutex::unlock()
{
    // Disable interrupts while updating mutex state
    bool interrupts_were_enabled = Machine::interrupts_enabled();
    if (interrupts_were_enabled)
    {
        Machine::disable_interrupts();
    }

    // Only the owner can unlock the mutex
    if (owner == Thread::CurrentThread())
    {
        locked = false;
        owner = nullptr;
    }
    else
    {
        // Error: Trying to unlock a mutex that this thread doesn't own
        Console::puts("ERROR: Thread trying to unlock a mutex it doesn't own!\n");
    }

    // Re-enable interrupts if they were enabled before
    if (interrupts_were_enabled)
    {
        Machine::enable_interrupts();
    }
}

bool Mutex::try_lock()
{
    // Disable interrupts while checking and updating mutex state
    bool interrupts_were_enabled = Machine::interrupts_enabled();
    if (interrupts_were_enabled)
    {
        Machine::disable_interrupts();
    }

    // If mutex is already locked, return false immediately
    if (locked)
    {
        if (interrupts_were_enabled)
        {
            Machine::enable_interrupts();
        }
        return false;
    }

    // Mutex is available, lock it
    locked = true;
    owner = Thread::CurrentThread();

    // Re-enable interrupts if they were enabled before
    if (interrupts_were_enabled)
    {
        Machine::enable_interrupts();
    }

    return true;
}