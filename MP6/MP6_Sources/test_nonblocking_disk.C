/*
     File        : test_nonblocking_disk.C

     Author      : [YOUR NAME]
     Date        : [CURRENT DATE]

     Description : Simple test program for NonBlockingDisk

*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "nonblocking_disk.H"
#include "system.H"
#include "thread.H"
#include "scheduler.H"
#include "machine.H"

// Global variables
Thread *test_thread1;
Thread *test_thread2;
Thread *test_thread3;

// Thread function for thread 1 - disk read/write
void thread1_func()
{
    Console::puts("Thread 1: Starting disk operations\n");

    // Create a buffer for disk operations
    unsigned char buf[512]; // 512 bytes = 1 disk block

    // Initialize buffer with some data
    for (int i = 0; i < 512; i++)
    {
        buf[i] = i % 256;
    }

    // Write to disk
    Console::puts("Thread 1: Writing to disk block 1\n");
    System::DISK->write(1, buf);
    Console::puts("Thread 1: Write completed\n");

    // Modify buffer data
    for (int i = 0; i < 512; i++)
    {
        buf[i] = 255 - (i % 256);
    }

    // Read from disk
    Console::puts("Thread 1: Reading from disk block 1\n");
    System::DISK->read(1, buf);
    Console::puts("Thread 1: Read completed\n");

    // Display a sample of the data
    Console::puts("Thread 1: Data sample: ");
    for (int i = 0; i < 10; i++)
    {
        Console::putui((unsigned int)buf[i]);
        Console::puts(" ");
    }
    Console::puts("\n");

    // Pass CPU to thread 2
    Console::puts("Thread 1: Passing CPU to Thread 2\n");
    System::SCHEDULER->resume(Thread::CurrentThread());
    System::SCHEDULER->yield();
}

// Thread function for thread 2 - console output
void thread2_func()
{
    Console::puts("Thread 2: Starting CPU-intensive work\n");

    // Simulate CPU-intensive work
    for (int i = 0; i < 5; i++)
    {
        Console::puts("Thread 2: Working... iteration ");
        Console::puti(i);
        Console::puts("\n");

        // Simulate work
        for (int j = 0; j < 1000000; j++)
        {
            // Empty loop to consume CPU cycles
        }
    }

    // Pass CPU to thread 3
    Console::puts("Thread 2: Passing CPU to Thread 3\n");
    System::SCHEDULER->resume(Thread::CurrentThread());
    System::SCHEDULER->yield();
}

// Thread function for thread 3 - more disk operations
void thread3_func()
{
    Console::puts("Thread 3: Starting disk operations\n");

    // Create a buffer for disk operations
    unsigned char buf[512]; // 512 bytes = 1 disk block

    // Read from disk
    Console::puts("Thread 3: Reading from disk block 1\n");
    System::DISK->read(1, buf);
    Console::puts("Thread 3: Read completed\n");

    // Display a sample of the data
    Console::puts("Thread 3: Data sample: ");
    for (int i = 0; i < 10; i++)
    {
        Console::putui((unsigned int)buf[i]);
        Console::puts(" ");
    }
    Console::puts("\n");

    // Pass CPU to thread 1
    Console::puts("Thread 3: Passing CPU to Thread 1\n");
    System::SCHEDULER->resume(Thread::CurrentThread());
    System::SCHEDULER->yield();
}

int main()
{
    // Initialize system components
    Console::init();

    // Create a disk
    System::DISK = new NonBlockingDisk(10 * 1024 * 1024); // 10MB disk

    // Create a scheduler
    System::SCHEDULER = new Scheduler();

    // Create threads
    Console::puts("Creating threads...\n");

    char *stack1 = new char[4096];
    test_thread1 = new Thread(thread1_func, stack1, 4096);

    char *stack2 = new char[4096];
    test_thread2 = new Thread(thread2_func, stack2, 4096);

    char *stack3 = new char[4096];
    test_thread3 = new Thread(thread3_func, stack3, 4096);

    Console::puts("Adding threads to scheduler...\n");

    // Add threads to scheduler
    System::SCHEDULER->add(test_thread1);
    System::SCHEDULER->add(test_thread2);
    System::SCHEDULER->add(test_thread3);

    // Start threads
    Console::puts("Starting thread 1...\n");
    Thread::dispatch_to(test_thread1);

    // If we get here, something went wrong
    assert(false);

    return 0;
}