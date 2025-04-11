/*
     File        : thread_safe_disk.C

     Author      : [YOUR NAME]
     Date        : [CURRENT DATE]

     Description : Thread-safe disk implementation that allows multiple threads
                   to safely access the disk concurrently.

*/

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

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
    // Acquire the disk state mutex to check disk state
    disk_state_mutex.lock();

    // Use NonBlockingDisk's method to check if the disk is busy
    if (!is_busy())
    {
        // Disk is not busy, no need to wait
        disk_state_mutex.unlock();
        return;
    }

    // Disk is busy, we'll need to wait
    disk_state_mutex.unlock();

    // Enable interrupts before yielding
    Machine::enable_interrupts();

    // Keep yielding until the disk is not busy
    while (true)
    {
        // Check if disk is still busy, with mutex protection
        disk_state_mutex.lock();
        bool still_busy = is_busy();
        disk_state_mutex.unlock();

        if (!still_busy)
        {
            // Disk is no longer busy, we can proceed
            break;
        }

        // Yield CPU to another thread and try again later
        Thread *current = Thread::CurrentThread();
        System::SCHEDULER->resume(current);
        System::SCHEDULER->yield();
    }
}

void ThreadSafeDisk::add_request_safe(unsigned long _block_no, unsigned char *_buffer, bool _is_read)
{
    // Lock the request queue mutex to protect the queue
    request_queue_mutex.lock();

    // Create a new request
    Thread *current = Thread::CurrentThread();
    ThreadSafeRequest *req = new ThreadSafeRequest(current, _block_no, _buffer, _is_read);

    // Add the request to the queue
    if (ts_pending_tail == nullptr)
    {
        ts_pending_head = ts_pending_tail = req;
    }
    else
    {
        ts_pending_tail->next = req;
        ts_pending_tail = req;
    }

    // Unlock the request queue mutex
    request_queue_mutex.unlock();
}

void ThreadSafeDisk::process_next_request_safe()
{
    // We need both mutexes here
    // First lock request queue to get the next request
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
        // Execute the read operation using the parent class's read function
        NonBlockingDisk::read(req->block_no, req->buffer);
    }
    else
    {
        // For write operations, use the copied buffer
        NonBlockingDisk::write(req->block_no, req->buffer_copy);
    }

    // Unlock disk state mutex
    disk_state_mutex.unlock();

    // Resume the thread that was waiting for this request
    System::SCHEDULER->resume(req->thread);

    // Free the request memory
    delete req;

    // Process the next request if there is one (recursively)
    // Note: We don't need a mutex here since we're just calling the method
    process_next_request_safe();
}

void ThreadSafeDisk::read(unsigned long _sector_number, unsigned char *_buffer)
{
    // Always ensure interrupts are enabled
    Machine::enable_interrupts();

    // First, lock the disk state mutex to check disk state
    disk_state_mutex.lock();

    // Check if disk is busy
    if (is_busy())
    {
        // Disk is busy, add request to queue and yield
        disk_state_mutex.unlock(); // Release mutex before queue operation

        // Add request to queue (this locks/unlocks the request queue mutex)
        add_request_safe(_sector_number, _buffer, true);

        // Yield CPU until our request is processed
        System::SCHEDULER->yield();
    }
    else
    {
        // Disk is not busy, perform the read directly

        // Use the NonBlockingDisk read method which handles polling
        NonBlockingDisk::read(_sector_number, _buffer);

        // Unlock disk state mutex
        disk_state_mutex.unlock();

        // Check if there are pending requests to process
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
    // Always ensure interrupts are enabled
    Machine::enable_interrupts();

    // First, lock the disk state mutex to check disk state
    disk_state_mutex.lock();

    // Check if disk is busy
    if (is_busy())
    {
        // Disk is busy, add request to queue and yield
        disk_state_mutex.unlock(); // Release mutex before queue operation

        // Add request to queue (this locks/unlocks the request queue mutex)
        add_request_safe(_sector_number, _buffer, false);

        // Yield CPU until our request is processed
        System::SCHEDULER->yield();
    }
    else
    {
        // Disk is not busy, perform the write directly

        // Use the NonBlockingDisk write method which handles polling
        NonBlockingDisk::write(_sector_number, _buffer);

        // Unlock disk state mutex
        disk_state_mutex.unlock();

        // Check if there are pending requests to process
        request_queue_mutex.lock();
        bool has_pending = (ts_pending_head != nullptr);
        request_queue_mutex.unlock();

        if (has_pending)
        {
            process_next_request_safe();
        }
    }
}