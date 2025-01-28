#include "frame_list.H"
#include "cont_frame_pool.H"
#include <cstddef>

void FrameList::init(unsigned long start, unsigned long length)
{
    head = (FreeBlock *)(start * FRAME_SIZE);
    head->start = start;
    head->length = length;
    head->next = NULL;
}

unsigned long FrameList::allocate(unsigned long n_frames)
{
    FreeBlock *curr = head, *prev = NULL;
    while (curr)
    {
        if (curr->length >= n_frames)
        {
            unsigned long alloc_start = curr->start;
            if (curr->length == n_frames)
            {
                if (prev)
                    prev->next = curr->next;
                else
                    head = curr->next;
            }
            else
            {
                curr->start += n_frames;
                curr->length -= n_frames;
            }
            return alloc_start;
        }
        prev = curr;
        curr = curr->next;
    }
    return 0;
}

void FrameList::release(unsigned long start, unsigned long length)
{
    FreeBlock *new_block = (FreeBlock *)(start * FRAME_SIZE);
    new_block->start = start;
    new_block->length = length;
    new_block->next = head;
    head = new_block;
}
