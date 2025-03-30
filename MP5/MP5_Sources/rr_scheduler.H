#ifndef RR_SCHEDULER_H
#define RR_SCHEDULER_H

#include "scheduler.H"
#include "simple_timer.H"

// Forward declaration
class RRScheduler;

class EOQTimer : public SimpleTimer
{
private:
    RRScheduler *scheduler;
    int tick_counter;
    int ticks_per_quantum;

public:
    EOQTimer(RRScheduler *_scheduler, int quantum_ms);
    virtual void handle_interrupt(REGS *_r);

    void restart();

    void set_ticks_per_quantum(int ticks) { ticks_per_quantum = ticks; }
};

class RRScheduler : public Scheduler
{
private:
    EOQTimer *timer;
    int quantum_ms;

public:
    RRScheduler(int _quantum_ms);

    virtual void yield();

    void end_of_quantum();

    EOQTimer *get_timer() { return timer; }
};

#endif