/*
     File        : nonblocking_disk.H

     Author      : [YOUR NAME]

     Date        : [CURRENT DATE]
     Description : Implementation of a non-blocking disk driver that uses a scheduler
                   instead of busy waiting when the disk is not ready.

*/

#ifndef _NONBLOCKING_DISK_H_
#define _NONBLOCKING_DISK_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "simple_disk.H"
#include "thread.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

// Forward declaration
class Scheduler;

// Structure for disk request queue
struct DiskRequest
{
   Thread *thread;         // Thread that made the request
   unsigned long block_no; // Block number to read/write
   unsigned char *buffer;  // Buffer for data
   bool is_read;           // true for read, false for write
   DiskRequest *next;      // Next request in the queue

   DiskRequest(Thread *t, unsigned long b, unsigned char *buf, bool r)
       : thread(t), block_no(b), buffer(buf), is_read(r), next(nullptr) {}
};

/*--------------------------------------------------------------------------*/
/* N o n B l o c K i n G D i s k  */
/*--------------------------------------------------------------------------*/

class NonBlockingDisk : public SimpleDisk
{
private:
   DiskRequest *pending_head; // Head of the request queue
   DiskRequest *pending_tail; // Tail of the request queue

   void add_request(unsigned long _block_no, unsigned char *_buffer, bool _is_read);
   void process_next_request();

public:
   NonBlockingDisk(unsigned int _size);
   /* Creates a NonBlockingDisk device with the given size connected to the
      MASTER slot of the primary ATA controller.
      NOTE: We are passing the _size argument out of laziness.
      In a real system, we would infer this information from the
      disk controller. */

   // Override the wait_while_busy method to yield CPU instead of busy waiting
   virtual void wait_while_busy() override;

   // Override read/write operations to use the request queue
   virtual void read(unsigned long _sector_number, unsigned char *_buffer) override;
   virtual void write(unsigned long _sector_number, unsigned char *_buffer) override;
};

#endif
