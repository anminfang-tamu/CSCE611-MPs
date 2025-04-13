/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "nonblocking_disk.H"
#include "system.H"
#include "thread.H"
#include "scheduler.H"
#include "machine.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

NonBlockingDisk::NonBlockingDisk(unsigned int _size)
    : SimpleDisk(_size), pending_head(nullptr), pending_tail(nullptr)
{
  Console::puts("Constructed NonBlockingDisk\n");
}

/*--------------------------------------------------------------------------*/
/* DISK OPERATIONS */
/*--------------------------------------------------------------------------*/

void NonBlockingDisk::wait_while_busy()
{
  // This function needs to yield when the disk is busy,
  // but always make sure interrupts are enabled when yielding
  while (is_busy())
  {
    // If disk is busy, yield CPU to another thread
    Thread *current = Thread::CurrentThread();

    // Make sure interrupts are enabled before yielding
    if (!Machine::interrupts_enabled())
    {
      Machine::enable_interrupts();
    }

    // Yield CPU to another thread
    System::SCHEDULER->resume(current);
    System::SCHEDULER->yield();

    // When we return from yield, we'll check if disk is still busy in the while condition
  }
}

void NonBlockingDisk::add_request(unsigned long _block_no, unsigned char *_buffer, bool _is_read)
{
  // Interrupts should already be disabled when this method is called

  // Create a new request
  Thread *current = Thread::CurrentThread();
  DiskRequest *req = new DiskRequest(current, _block_no, _buffer, _is_read);

  // Add the request to the queue
  if (pending_tail == nullptr)
  {
    pending_head = pending_tail = req;
  }
  else
  {
    pending_tail->next = req;
    pending_tail = req;
  }
}

void NonBlockingDisk::process_next_request()
{
  // Make sure interrupts are enabled at the beginning
  if (!Machine::interrupts_enabled())
  {
    Machine::enable_interrupts();
  }

  // Make sure we have a request to process
  if (pending_head == nullptr)
  {
    return;
  }

  // Disable interrupts while modifying the queue
  Machine::disable_interrupts();

  // Get the next request
  DiskRequest *req = pending_head;
  pending_head = pending_head->next;
  if (pending_head == nullptr)
  {
    pending_tail = nullptr;
  }

  // Save request details before enabling interrupts
  Thread *request_thread = req->thread;
  unsigned long block_no = req->block_no;
  unsigned char *buffer = req->buffer;
  bool is_read = req->is_read;

  // Enable interrupts for the disk operation
  Machine::enable_interrupts();

  // Process the request
  if (is_read)
  {
    // Execute the read operation using the parent class's read function
    SimpleDisk::read(block_no, buffer);
  }
  else
  {
    // Execute the write operation using the parent class's write function
    SimpleDisk::write(block_no, buffer);
  }

  // Resume the thread that was waiting for this request
  System::SCHEDULER->resume(request_thread);

  // Free the request memory
  delete req;

  // Process the next request if there is one
  if (pending_head != nullptr)
  {
    process_next_request();
  }
}

void NonBlockingDisk::read(unsigned long _sector_number, unsigned char *_buffer)
{
  // Always enable interrupts first if they're not already enabled
  if (!Machine::interrupts_enabled())
  {
    Machine::enable_interrupts();
  }

  // Disable interrupts while checking the disk state and updating the queue
  Machine::disable_interrupts();

  // Check if disk is busy or there are pending requests
  if (is_busy() || pending_head != nullptr)
  {
    // Add this read request to the queue
    add_request(_sector_number, _buffer, true);

    // Enable interrupts before yielding
    Machine::enable_interrupts();

    // Yield CPU until our request is processed
    System::SCHEDULER->yield();
  }
  else
  {
    // Disk is not busy and queue is empty, perform the read directly

    // Enable interrupts before the operation
    Machine::enable_interrupts();

    // Perform the read operation
    SimpleDisk::read(_sector_number, _buffer);

    // After performing the direct operation, check if there are any pending requests
    // that can now be processed
    if (pending_head != nullptr)
    {
      process_next_request();
    }
  }
}

void NonBlockingDisk::write(unsigned long _sector_number, unsigned char *_buffer)
{
  // Always enable interrupts first if they're not already enabled
  if (!Machine::interrupts_enabled())
  {
    Machine::enable_interrupts();
  }

  // Disable interrupts while checking the disk state and updating the queue
  Machine::disable_interrupts();

  // Check if disk is busy or there are pending requests
  if (is_busy() || pending_head != nullptr)
  {
    // Add this write request to the queue
    add_request(_sector_number, _buffer, false);

    // Enable interrupts before yielding
    Machine::enable_interrupts();

    // Yield CPU until our request is processed
    System::SCHEDULER->yield();
  }
  else
  {
    // Disk is not busy and queue is empty, perform the write directly

    // Enable interrupts before the operation
    Machine::enable_interrupts();

    // Perform the write operation
    SimpleDisk::write(_sector_number, _buffer);

    // After performing the direct operation, check if there are any pending requests
    // that can now be processed
    if (pending_head != nullptr)
    {
      process_next_request();
    }
  }
}