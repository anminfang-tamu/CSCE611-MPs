/*
 File: scheduler.C

 Author:
 Date  :

 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "machine.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

Scheduler::Scheduler()
{
  head = nullptr;
  tail = nullptr;
  Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield()
{
  Thread *current = Thread::CurrentThread();
  Thread *next;

  // Disable interrupts during scheduling decisions
  Machine::disable_interrupts();

  if (head == nullptr)
  {
    // If no other threads are ready, keep running the current thread
    // or panic if there's no current thread either
    if (current == nullptr)
    {
      Console::puts("ERROR: No threads to run!\n");
      assert(false);
    }
    // Enable interrupts before returning
    Machine::enable_interrupts();
    return;
  }

  // Get the next thread from the front of the ready queue
  next = head->thread;
  ThreadNode *old_head = head;
  head = head->next;
  if (head == nullptr)
  {
    tail = nullptr;
  }
  delete old_head;

  // Add the current thread to the back of the ready queue, but only if
  // there is a current thread (not the case when starting the first thread)
  if (current != nullptr)
  {
    ThreadNode *new_node = new ThreadNode(current);
    if (tail == nullptr)
    {
      head = tail = new_node;
    }
    else
    {
      tail->next = new_node;
      tail = new_node;
    }
  }

  // Enable interrupts before dispatching
  Machine::enable_interrupts();

  // Switch to the next thread
  Thread::dispatch_to(next);
}

void Scheduler::resume(Thread *_thread)
{
  // Add the thread to the back of the ready queue
  Machine::disable_interrupts();

  // Don't add the thread if it's already in the queue
  ThreadNode *current = head;
  while (current != nullptr)
  {
    if (current->thread == _thread)
    {
      // Thread is already in the queue
      Machine::enable_interrupts();
      return;
    }
    current = current->next;
  }

  ThreadNode *new_node = new ThreadNode(_thread);
  if (tail == nullptr)
  {
    head = tail = new_node;
  }
  else
  {
    tail->next = new_node;
    tail = new_node;
  }

  Machine::enable_interrupts();
}

void Scheduler::add(Thread *_thread)
{
  // For FIFO, this is the same as resume
  Console::puts("Adding thread ");
  Console::puti(_thread->ThreadId());
  Console::puts(" to scheduler\n");
  resume(_thread);
}

void Scheduler::terminate(Thread *_thread)
{
  Thread *current = Thread::CurrentThread();

  Machine::disable_interrupts();

  // If we're terminating the current thread
  if (_thread == current)
  {
    Thread *next;

    if (head == nullptr)
    {
      // No more threads to run! System should halt or panic
      Console::puts("Last thread terminated. System halting.\n");
      // Halt the system or enter an infinite loop
      for (;;)
        ;
    }

    // Get the next thread from the front of the ready queue
    next = head->thread;
    ThreadNode *old_head = head;
    head = head->next;
    if (head == nullptr)
    {
      tail = nullptr;
    }
    delete old_head;

    // Don't add the current thread back to ready queue since it's terminating
    // Just switch to the next thread
    Thread::dispatch_to(next);
  }
  else
  {
    // Remove the thread from the ready queue if it's there
    ThreadNode *prev = nullptr;
    ThreadNode *current = head;

    while (current != nullptr)
    {
      if (current->thread == _thread)
      {
        if (prev == nullptr)
        {
          // It's the head
          head = current->next;
          if (head == nullptr)
          {
            tail = nullptr;
          }
        }
        else
        {
          // It's in the middle or at the end
          prev->next = current->next;
          if (current == tail)
          {
            tail = prev;
          }
        }
        delete current;
        break;
      }
      prev = current;
      current = current->next;
    }
  }

  Machine::enable_interrupts();

  // TODO: Free resources associated with the terminated thread
  // This is tricky since the thread can't free its own stack while using it
}
