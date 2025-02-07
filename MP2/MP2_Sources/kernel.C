/*
    File: kernel.C

    Author: R. Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 2024/08/20


    This file has the main entry point to the operating system.

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define MB *(0x1 << 20)
#define KB *(0x1 << 10)
/* Makes things easy to read */

#define KERNEL_POOL_START_FRAME ((2 MB) / (4 KB))
#define KERNEL_POOL_SIZE ((2 MB) / (4 KB))
#define PROCESS_POOL_START_FRAME ((4 MB) / (4 KB))
#define PROCESS_POOL_SIZE ((28 MB) / (4 KB))
/* Definition of the kernel and process memory pools */

#define MEM_HOLE_START_FRAME ((15 MB) / (4 KB))
#define MEM_HOLE_SIZE ((1 MB) / (4 KB))
/* We have a 1 MB hole in physical memory starting at address 15 MB */

#define TEST_START_ADDR_PROC (4 MB)
#define TEST_START_ADDR_KERNEL (2 MB)
/* Used in the memory test below to generate sequences of memory references. */
/* One is for a sequence of memory references in the kernel space, and the   */
/* other for memory references in the process space. */

#define N_TEST_ALLOCATIONS 32
/* Number of recursive allocations that we use to test.  */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "machine.H" /* LOW-LEVEL STUFF   */
#include "console.H"

#include "assert.H"
#include "cont_frame_pool.H" /* The physical memory manager */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

void test_memory(ContFramePool *_pool, unsigned int _allocs_to_go);
void failed_test_frame_pool();
void successful_test_frame_pool();
void test_fragmentation(ContFramePool *pool);
void test_small_allocations(ContFramePool *pool);
void test_medium_allocations(ContFramePool *pool);
void test_sequential_allocation(ContFramePool *pool);
/*--------------------------------------------------------------------------*/
/* MAIN ENTRY INTO THE OS */
/*--------------------------------------------------------------------------*/

int main()
{

    Console::init();
    Console::redirect_output(true); // comment if you want to stop redirecting qemu window output to stdout

    /* -- INITIALIZE FRAME POOLS -- */

    /* ---- KERNEL POOL -- */

    ContFramePool kernel_mem_pool(KERNEL_POOL_START_FRAME,
                                  KERNEL_POOL_SIZE,
                                  0);

    /* ---- PROCESS POOL -- */

    /*  // In later machine problems, we will be using two pools. You may want to comment this out and test
        // the management of two pools.

        unsigned long n_info_frames = ContFramePool::needed_info_frames(PROCESS_POOL_SIZE);

        unsigned long process_mem_pool_info_frame = kernel_mem_pool.get_frames(n_info_frames);

        ContFramePool process_mem_pool(PROCESS_POOL_START_FRAME,
                                       PROCESS_POOL_SIZE,
                                       process_mem_pool_info_frame);

        process_mem_pool.mark_inaccessible(MEM_HOLE_START_FRAME, MEM_HOLE_SIZE);
    */

    /* -- MOST OF WHAT WE NEED IS SETUP. THE KERNEL CAN START. */

    Console::puts("Hello World!\n");

    /* -- TEST MEMORY ALLOCATOR */

    test_memory(&kernel_mem_pool, N_TEST_ALLOCATIONS);

    /* ---- Add code here to test the frame pool implementation. */

    /* -- NOW LOOP FOREVER */
    Console::puts("Testing is DONE. We will do nothing forever\n");
    Console::puts("Feel free to turn off the machine now.\n");
    Console::puts("==============================================\n");

    // Additional tests
    failed_test_frame_pool();
    successful_test_frame_pool();
    test_fragmentation(&kernel_mem_pool);
    test_small_allocations(&kernel_mem_pool);
    test_medium_allocations(&kernel_mem_pool);
    test_sequential_allocation(&kernel_mem_pool);

    for (;;)
        ;

    /* -- WE DO THE FOLLOWING TO KEEP THE COMPILER HAPPY. */
    return 1;
}

void test_memory(ContFramePool *_pool, unsigned int _allocs_to_go)
{
    Console::puts("alloc_to_go = ");
    Console::puti(_allocs_to_go);
    Console::puts("\n");
    if (_allocs_to_go > 0)
    {
        // We have not reached the end yet.
        int n_frames = _allocs_to_go % 4 + 1;              // number of frames you want to allocate
        unsigned long frame = _pool->get_frames(n_frames); // we allocate the frames from the pool
        int *value_array = (int *)(frame * (4 KB));        // we pick a unique number that we want to write into the memory we just allocated
        for (int i = 0; i < (1 KB) * n_frames; i++)
        { // we write this value int the memory locations
            value_array[i] = _allocs_to_go;
        }
        test_memory(_pool, _allocs_to_go - 1); // recursively allocate and uniquely mark more memory
        for (int i = 0; i < (1 KB) * n_frames; i++)
        { // We check the values written into the memory before we recursed
            if (value_array[i] != _allocs_to_go)
            { // If the value stored in the memory locations is not the same that we wrote a few lines above
              // then somebody overwrote the memory.
                Console::puts("MEMORY TEST FAILED. ERROR IN FRAME POOL\n");
                Console::puts("i =");
                Console::puti(i);
                Console::puts("   v = ");
                Console::puti(value_array[i]);
                Console::puts("   n =");
                Console::puti(_allocs_to_go);
                Console::puts("\n");
                for (;;)
                    ; // We throw a fit.
            }
        }
        ContFramePool::release_frames(frame);
    }
}

// Failed test case for frame pool
void failed_test_frame_pool()
{
    Console::puts("Failed test case for frame pool\n");
    ContFramePool frame_pool(0, 10, 0);
    frame_pool.mark_inaccessible(0, 10);
    Console::puts("frame_pool.get_frames(1) returned: ");
    Console::puti(frame_pool.get_frames(1));
    Console::puts("\n");
    frame_pool.release_frames(0);
    Console::puts("==============================================\n");
}

// Successful test case for frame pool
void successful_test_frame_pool()
{
    Console::puts("Successful test case for frame pool\n");

    ContFramePool frame_pool(10, 10, 0);
    frame_pool.mark_inaccessible(10, 5);

    Console::puts("frame_pool.get_frames(1) returned: ");
    unsigned long frame = frame_pool.get_frames(1); // Should succeed, using frame 15-19
    Console::puti(frame);
    Console::puts("\n");
    if (frame != 0)
    {
        frame_pool.release_frames(frame);
    }
    Console::puts("==============================================\n");
}

void test_fragmentation(ContFramePool *pool)
{
    Console::puts("\nTesting fragmentation scenarios:\n");

    // Allocate multiple small chunks
    unsigned long frames[5];
    for (int i = 0; i < 5; i++)
    {
        frames[i] = pool->get_frames(2); // Allocate 2 frames each time
        Console::puts("Allocated 2 frames at: ");
        Console::puti(frames[i]);
        Console::puts("\n");
    }

    // Release alternate frames to create fragmentation
    for (int i = 0; i < 5; i += 2)
    {
        ContFramePool::release_frames(frames[i]);
        Console::puts("Released frames at: ");
        Console::puti(frames[i]);
        Console::puts("\n");
    }

    // Try to allocate a larger chunk
    Console::puts("Attempting to allocate 4 contiguous frames: ");
    unsigned long large_frame = pool->get_frames(4);
    if (large_frame == 0)
    {
        Console::puts("Failed due to fragmentation (expected)\n");
    }
    else
    {
        Console::puts("Succeeded at: ");
        Console::puti(large_frame);
        Console::puts("\n");
        ContFramePool::release_frames(large_frame);
    }

    // Clean up remaining allocations
    for (int i = 1; i < 5; i += 2)
    {
        ContFramePool::release_frames(frames[i]);
    }
    Console::puts("==============================================\n");
}

void test_small_allocations(ContFramePool *pool)
{
    Console::puts("\nTesting small allocations:\n");

    // Test single frame allocations
    Console::puts("Testing single frame allocations:\n");
    unsigned long frames[3];

    // Allocation phase
    for (int i = 0; i < 3; i++)
    {
        frames[i] = pool->get_frames(1);
        Console::puts("Allocated frame at: ");
        Console::puti(frames[i]);
        Console::puts("\n");

        // Verify frame is within expected range
        if (frames[i] < KERNEL_POOL_START_FRAME ||
            frames[i] >= (KERNEL_POOL_START_FRAME + KERNEL_POOL_SIZE))
        {
            Console::puts("ERROR: Frame outside valid range!\n");
        }
    }

    // Verify sequential allocation
    for (int i = 1; i < 3; i++)
    {
        if (frames[i] != frames[i - 1] + 1)
        {
            Console::puts("ERROR: Frames not sequential!\n");
        }
    }

    // Release phase
    for (int i = 0; i < 3; i++)
    {
        Console::puts("Releasing frame: ");
        Console::puti(frames[i]);
        Console::puts("\n");
        ContFramePool::release_frames(frames[i]);
    }

    // Reallocation test
    unsigned long new_frame = pool->get_frames(1);
    Console::puts("Reallocated frame at: ");
    Console::puti(new_frame);
    Console::puts("\n");

    // Verify reallocation uses a previously freed frame
    bool found_match = false;
    for (int i = 0; i < 3; i++)
    {
        if (new_frame == frames[i])
        {
            found_match = true;
            break;
        }
    }
    if (!found_match)
    {
        Console::puts("WARNING: Reallocated frame was not one of the previously freed frames\n");
    }

    ContFramePool::release_frames(new_frame);
    Console::puts("==============================================\n");
}

void test_medium_allocations(ContFramePool *pool)
{
    Console::puts("\nTesting medium allocations:\n");

    // Try to allocate half of available frames
    unsigned int half_size = KERNEL_POOL_SIZE / 2;
    Console::puts("Attempting to allocate ");
    Console::puti(half_size);
    Console::puts(" frames: ");

    unsigned long half_frames = pool->get_frames(half_size);
    if (half_frames != 0)
    {
        Console::puts("Success at frame: ");
        Console::puti(half_frames);
        Console::puts("\n");
        ContFramePool::release_frames(half_frames);
    }
    else
    {
        Console::puts("Failed\n");
    }
    Console::puts("==============================================\n");
}

void test_sequential_allocation(ContFramePool *pool)
{
    Console::puts("\nTesting sequential allocations:\n");

    // Allocate 3 frames sequentially
    unsigned long frame1 = pool->get_frames(1);
    unsigned long frame2 = pool->get_frames(1);
    unsigned long frame3 = pool->get_frames(1);

    Console::puts("Sequential frames: ");
    Console::puti(frame1);
    Console::puts(", ");
    Console::puti(frame2);
    Console::puts(", ");
    Console::puti(frame3);
    Console::puts("\n");

    // Release in reverse order
    ContFramePool::release_frames(frame3);
    ContFramePool::release_frames(frame2);
    ContFramePool::release_frames(frame1);

    Console::puts("==============================================\n");
}
