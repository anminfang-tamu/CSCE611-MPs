#include "rr_scheduler.H"
#include "console.H"
#include "thread.H"
#include "machine.H"
#include "interrupts.H"

// Global variable to track if a thread should be preempted
static Thread *thread_to_preempt = nullptr;

EOQTimer::EOQTimer(RRScheduler *_scheduler, int quantum_ms)
    : SimpleTimer(100), // Set the timer to 100Hz (10ms intervals)
      scheduler(_scheduler),
      tick_counter(0),
      ticks_per_quantum(quantum_ms / 10)
{
    // Make sure we have at least 1 tick per quantum
    if (ticks_per_quantum < 1)
        ticks_per_quantum = 1;

    Console::puts("EOQ Timer initialized at 100Hz\n");
}

void EOQTimer::handle_interrupt(REGS *_r)
{
    SimpleTimer::handle_interrupt(_r);

    // Track timer ticks for debugging
    static int debug_counter = 0;
    debug_counter++;

    // Print occasional debug messages
    if (debug_counter % 20 == 0)
    {
        Console::puts("Timer interrupt #");
        Console::puti(debug_counter);
        Console::puts("\n");
    }

    // Increment quantum tick counter
    tick_counter++;

    // Check if quantum has expired
    if (tick_counter >= ticks_per_quantum)
    {
        // Reset the tick counter
        tick_counter = 0;

        // Get the current thread
        Thread *current = Thread::CurrentThread();
        if (current != nullptr)
        {
            Console::puts("\n*** QUANTUM EXPIRED - PREEMPTING THREAD ***\n");

            thread_to_preempt = current;

            Console::puts("Marking thread ");
            Console::puti(thread_to_preempt->ThreadId());
            Console::puts(" for preemption\n");
        }
        else
        {
            Console::puts("No current thread to preempt\n");
        }
    }
}

void EOQTimer::restart()
{
    tick_counter = 0;
}

RRScheduler::RRScheduler(int _quantum_ms)
    : Scheduler(), quantum_ms(_quantum_ms)
{
    Console::puts("Creating EOQ Timer...\n");

    timer = new EOQTimer(this, quantum_ms);

    int ticks = quantum_ms / 10;
    if (ticks < 1)
        ticks = 1;

    Console::puts("Round-Robin Scheduler initialized with quantum = ");
    Console::puti(quantum_ms);
    Console::puts(" ms (");
    Console::puti(ticks);
    Console::puts(" ticks)\n");
}

void RRScheduler::yield()
{
    // Check if a thread is marked for preemption
    if (thread_to_preempt != nullptr && thread_to_preempt == Thread::CurrentThread())
    {
        Console::puts("Handling deferred preemption for thread ");
        Console::puti(thread_to_preempt->ThreadId());
        Console::puts("\n");

        // Clear the preemption mark
        thread_to_preempt = nullptr;

        // Reset timer
        timer->restart();
    }
    else
    {
        timer->restart();
    }

    Scheduler::yield();
}

void RRScheduler::end_of_quantum()
{
    Console::puts("end_of_quantum: This method should not be called anymore\n");
    yield();
}