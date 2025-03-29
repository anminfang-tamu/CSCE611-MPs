#include "rr_scheduler.H"
#include "thread.H"
#include "console.H"
#include "machine.H"

// RRScheduler implementation
RRScheduler::RRScheduler(int quantum_ms) : Scheduler()
{
    // Create and start the EOQ timer
    timer = new EOQTimer(this, quantum_ms);
    timer->start();
    Console::puts("Constructed Round-Robin Scheduler with quantum = ");
    Console::puti(quantum_ms);
    Console::puts(" ms\n");
}

void RRScheduler::yield()
{
    // Stop the timer before yielding to avoid double-preemption
    timer->reset();

    // Call the base class yield
    Scheduler::yield();

    // The timer will restart when we get control back
}

void RRScheduler::end_of_quantum()
{
    Thread *current = Thread::CurrentThread();

    // Force the current thread to yield
    Console::puts("Quantum expired for thread ");
    Console::puti(current->ThreadId());
    Console::puts("\n");

    // Resume the current thread (puts it back in ready queue)
    resume(current);

    // Get the next thread to run
    if (!ready_queue->empty())
    {
        Thread *next = ready_queue->front();
        ready_queue->pop_front();

        // Switch to next thread
        Thread::dispatch_to(next);
    }
}

// EOQTimer implementation
EOQTimer::EOQTimer(RRScheduler *_scheduler, int quantum_ms)
    : SimpleTimer(quantum_ms), scheduler(_scheduler)
{
}

void EOQTimer::handle_interrupt(REGS *_r)
{
    // Call the base class handler to reset the timer
    SimpleTimer::handle_interrupt(_r);

    // Call the scheduler's end_of_quantum method
    scheduler->end_of_quantum();
}

void EOQTimer::reset()
{
    // Call the parent class's reset method
    SimpleTimer::reset();
}

void EOQTimer::start()
{
    // Call the parent class's start method
    SimpleTimer::start();
}