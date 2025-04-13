#include "thread_safe_disk.H"
#include "assert.H"
#include "utils.H"
#include "console.H"
#include "system.H"
#include "thread.H"
#include "machine.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

ThreadSafeDisk::ThreadSafeDisk(unsigned int _size)
    : NonBlockingDisk(_size), ts_pending_head(nullptr), ts_pending_tail(nullptr)
{
    Console::puts("Constructed ThreadSafeDisk\n");
}

/*--------------------------------------------------------------------------*/
/* DISK OPERATIONS */
/*--------------------------------------------------------------------------*/

void ThreadSafeDisk::wait_while_busy()
{
    disk_state_mutex.lock();

    if (!is_busy())
    {
        disk_state_mutex.unlock();
        return;
    }

    disk_state_mutex.unlock();

    // Only enable interrupts if they're not already enabled
    if (!Machine::interrupts_enabled())
    {
        Machine::enable_interrupts();
    }

    while (true)
    {
        disk_state_mutex.lock();
        bool still_busy = is_busy();
        disk_state_mutex.unlock();

        if (!still_busy)
        {
            break;
        }

        Thread *current = Thread::CurrentThread();
        System::SCHEDULER->resume(current);
        System::SCHEDULER->yield();
    }
}

void ThreadSafeDisk::add_request_safe(unsigned long _block_no, unsigned char *_buffer, bool _is_read)
{
    request_queue_mutex.lock();

    Thread *current = Thread::CurrentThread();
    ThreadSafeRequest *req = new ThreadSafeRequest(current, _block_no, _buffer, _is_read);

    if (ts_pending_tail == nullptr)
    {
        ts_pending_head = ts_pending_tail = req;
    }
    else
    {
        ts_pending_tail->next = req;
        ts_pending_tail = req;
    }

    request_queue_mutex.unlock();
}

void ThreadSafeDisk::process_next_request_safe()
{
    request_queue_mutex.lock();

    // Make sure we have a request to process
    if (ts_pending_head == nullptr)
    {
        request_queue_mutex.unlock();
        return;
    }

    // Get the next request from the queue
    ThreadSafeRequest *req = ts_pending_head;
    ts_pending_head = ts_pending_head->next;
    if (ts_pending_head == nullptr)
    {
        ts_pending_tail = nullptr;
    }

    // Unlock request queue mutex as we're done with the queue
    request_queue_mutex.unlock();

    // Now lock disk state mutex to perform disk operation
    disk_state_mutex.lock();

    // Process the request
    if (req->is_read)
    {
        NonBlockingDisk::read(req->block_no, req->buffer);
    }
    else
    {
        NonBlockingDisk::write(req->block_no, req->buffer_copy);
    }

    disk_state_mutex.unlock();

    System::SCHEDULER->resume(req->thread);

    delete req;

    process_next_request_safe();
}

void ThreadSafeDisk::read(unsigned long _sector_number, unsigned char *_buffer)
{
    // Only enable interrupts if they're not already enabled
    if (!Machine::interrupts_enabled())
    {
        Machine::enable_interrupts();
    }

    disk_state_mutex.lock();

    if (is_busy())
    {
        disk_state_mutex.unlock();

        add_request_safe(_sector_number, _buffer, true);

        System::SCHEDULER->yield();
    }
    else
    {
        NonBlockingDisk::read(_sector_number, _buffer);

        disk_state_mutex.unlock();

        request_queue_mutex.lock();
        bool has_pending = (ts_pending_head != nullptr);
        request_queue_mutex.unlock();

        if (has_pending)
        {
            process_next_request_safe();
        }
    }
}

void ThreadSafeDisk::write(unsigned long _sector_number, unsigned char *_buffer)
{
    // Only enable interrupts if they're not already enabled
    if (!Machine::interrupts_enabled())
    {
        Machine::enable_interrupts();
    }

    disk_state_mutex.lock();

    if (is_busy())
    {
        disk_state_mutex.unlock();

        add_request_safe(_sector_number, _buffer, false);

        System::SCHEDULER->yield();
    }
    else
    {
        NonBlockingDisk::write(_sector_number, _buffer);

        disk_state_mutex.unlock();

        request_queue_mutex.lock();
        bool has_pending = (ts_pending_head != nullptr);
        request_queue_mutex.unlock();

        if (has_pending)
        {
            process_next_request_safe();
        }
    }
}