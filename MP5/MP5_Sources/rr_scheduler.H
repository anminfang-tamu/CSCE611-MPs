#ifndef RR_SCHEDULER_H
#define RR_SCHEDULER_H

#include "scheduler.H"
#include "simple_timer.H"

class EOQTimer; // Forward declaration

class RRScheduler : public Scheduler
{
private:
    EOQTimer *timer; // End-of-quantum timer

public:
    RRScheduler(int quantum_ms); // Constructor with quantum size in milliseconds

    // Override the yield method to handle quantum reset
    virtual void yield();

    // Method called by timer when quantum expires
    void end_of_quantum();
};

// EOQ Timer class
class EOQTimer : public SimpleTimer
{
private:
    RRScheduler *scheduler;

public:
    EOQTimer(RRScheduler *_scheduler, int quantum_ms);

    // Override the interrupt handler to call scheduler's end_of_quantum
    virtual void handle_interrupt(REGS *_r);

    void reset();
    void start();
};

#endif