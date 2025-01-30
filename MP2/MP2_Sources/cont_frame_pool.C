/*
 File: ContFramePool.C

 Author: Anmin Fang
 Date  : 2025-01-28

 */

/*--------------------------------------------------------------------------*/
/*
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates
 *single* frames at a time. Because it does allocate one frame at a time,
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.

 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.

 This can be done in many ways, ranging from extensions to bitmaps to
 free-lists of frames etc.

 IMPLEMENTATION:

 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame,
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool.
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.

 NOTE: If we use this scheme to allocate only single frames, then all
 frames are marked as either FREE or HEAD-OF-SEQUENCE.

 NOTE: In SimpleFramePool we needed only one bit to store the state of
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work,
 revisit the implementation and change it to using two bits. You will get
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.

 DETAILED IMPLEMENTATION:

 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:

 Constructor: Initialize all frames to FREE, except for any frames that you
 need for the management of the frame pool, if any.

 get_frames(_n_frames): Traverse the "bitmap" of states and look for a
 sequence of at least _n_frames entries that are FREE. If you find one,
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.

 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.

 needed_info_frames(_n_frames): This depends on how many bits you need
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.

 A WORD ABOUT RELEASE_FRAMES():

 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e.,
 not associated with a particular frame pool.

 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete

 */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "frame_list.H"

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
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/

// Initialize static member
ContFramePool *ContFramePool::first_pool = nullptr;

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no)
{
    Console::puts("ContframePool::Constructor implemented!\n");
    Console::puts("base_frame_no: ");
    Console::puti(_base_frame_no);
    Console::puts("\n");
    Console::puts("n_frames: ");
    Console::puti(_n_frames);
    Console::puts("\n");
    Console::puts("info_frame_no: ");
    Console::puti(_info_frame_no);
    Console::puts("\n");

    base_frame_no = _base_frame_no;
    nframes = _n_frames;
    info_frame_no = _info_frame_no;
    allocated_blocks = nullptr;
    allocated_block_offset = 0;

    // Step 1: figure out how many frames are needed for metadata
    unsigned long needed = needed_info_frames(nframes);

    // Step 2: decide where to place the info frames in the pool
    unsigned long info_start; // We'll store the first metadata frame
    if (info_frame_no == 0)
    {
        // Step 3: We place the metadata frames at the front
        info_start = base_frame_no;
        // Step 4: Then remove those frames from the "real" pool
        base_frame_no += needed;
        nframes -= needed;
    }
    else
    {
        // Step 5: If the user gave us an explicit frame for metadata
        info_start = info_frame_no;
    }

    void *free_list_storage = (void *)((unsigned long)info_start * Machine::PAGE_SIZE);

    free_list.init_managed_region(base_frame_no, nframes, free_list_storage);

    unsigned long frame_list_usage = sizeof(FrameList::FreeBlock);

    allocated_block_offset = frame_list_usage;

    next_pool = first_pool;
    first_pool = this;
}

unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
    // Step 1: Allocate frames from the free_list
    unsigned long frame_no = free_list.allocate(_n_frames);
    if (frame_no == 0)
    {
        return 0; // fails
    }

    // Step 2: Place an AllocatedBlock in the metadata region
    AllocatedBlock *new_block =
        (AllocatedBlock *)((info_frame_no * Machine::PAGE_SIZE) + allocated_block_offset);

    // Step 3: Store frame_no as relative or absolute, but must be consistent in release_frames().
    new_block->frame_no = frame_no - base_frame_no; // store relative
    new_block->length = _n_frames;
    new_block->next = allocated_blocks;
    allocated_blocks = new_block;

    allocated_block_offset += sizeof(AllocatedBlock);

    // Step 4: Return absolute frame number
    return frame_no;
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no, unsigned long _n_frames)
{
    free_list.mark_unavailable(_base_frame_no, _n_frames);
}

void ContFramePool::release_frames(unsigned long _first_frame_no)
{
    // Step 1: Find which ContFramePool this frame belongs to
    ContFramePool *pool = first_pool;
    while (pool)
    {
        unsigned long start = pool->base_frame_no;
        unsigned long end_plus = start + pool->nframes;

        if (_first_frame_no >= start && _first_frame_no < end_plus)
        {
            // Found the correct pool
            AllocatedBlock **curr = &pool->allocated_blocks;
            while (*curr)
            {
                if ((*curr)->frame_no == (_first_frame_no - start))
                {
                    unsigned int length = (*curr)->length;
                    AllocatedBlock *dead = *curr;
                    *curr = (*curr)->next;

                    // Step 2: Return frames to free_list
                    pool->free_list.release(_first_frame_no, length);
                    return;
                }
                curr = &((*curr)->next);
            }
            Console::puts("Warning: Could not find allocation record for frame\n");
            return;
        }
        pool = pool->next_pool;
    }
}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    // Step 1: Calculate the number of bytes needed for metadata
    unsigned long block_bytes =
        (sizeof(FrameList::FreeBlock) + sizeof(AllocatedBlock)) * _n_frames;

    // Step 2: Convert bytes to frames
    unsigned long frames = (block_bytes + FRAME_SIZE - 1) / FRAME_SIZE;

    if (frames == 0)
    {
        frames = 1;
    }
    return frames;
}
