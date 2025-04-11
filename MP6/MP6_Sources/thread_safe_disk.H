/*
     File        : thread_safe_disk.H

     Author      : [YOUR NAME]
     Date        : [CURRENT DATE]

     Description : Thread-safe disk implementation that allows multiple threads
                   to safely access the disk concurrently.

*/

#ifndef _THREAD_SAFE_DISK_H_
#define _THREAD_SAFE_DISK_H_

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "nonblocking_disk.H"
#include "mutex.H"
#include "thread.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

struct ThreadSafeRequest
{
    Thread *thread;                 // Thread that made the request
    unsigned long block_no;         // Block number to read/write
    unsigned char *buffer;          // Buffer pointer for the thread
    unsigned char buffer_copy[512]; // Copy of buffer for write operations
    bool is_read;                   // true for read, false for write
    ThreadSafeRequest *next;        // Next request in the queue

    ThreadSafeRequest(Thread *t, unsigned long b, unsigned char *buf, bool r)
        : thread(t), block_no(b), buffer(buf), is_read(r), next(nullptr)
    {
        // For write operations, make a copy of the buffer
        if (!r)
        {
            for (int i = 0; i < 512; i++)
            {
                buffer_copy[i] = buf[i];
            }
        }
    }
};

/*--------------------------------------------------------------------------*/
/* T h r e a d S a f e D i s k */
/*--------------------------------------------------------------------------*/

class ThreadSafeDisk : public NonBlockingDisk
{
private:
    Mutex request_queue_mutex; // Protects the request queue
    Mutex disk_state_mutex;    // Protects disk state and operations

    ThreadSafeRequest *ts_pending_head; // Head of the thread-safe request queue
    ThreadSafeRequest *ts_pending_tail; // Tail of the thread-safe request queue

    // Private helper methods
    void add_request_safe(unsigned long _block_no, unsigned char *_buffer, bool _is_read);
    void process_next_request_safe();

public:
    ThreadSafeDisk(unsigned int _size);
    /* Creates a ThreadSafeDisk device with the given size connected to the
       MASTER slot of the primary ATA controller. */

    // Override NonBlockingDisk methods to add thread safety
    virtual void wait_while_busy() override;
    virtual void read(unsigned long _sector_number, unsigned char *_buffer) override;
    virtual void write(unsigned long _sector_number, unsigned char *_buffer) override;
};

#endif