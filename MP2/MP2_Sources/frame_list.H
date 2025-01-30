#ifndef _FRAME_LIST_H_
#define _FRAME_LIST_H_

#include "machine.H"

class FrameList
{
public:
    struct FreeBlock
    {
        unsigned long start;
        unsigned long length;
        FreeBlock *next;
    };

    // Default constructor: no free blocks
    FrameList() : head(nullptr) {}

    void init_managed_region(unsigned long start, unsigned long length, void *storage);
    unsigned long allocate(unsigned long n_frames);
    void release(unsigned long start, unsigned long length);
    void mark_unavailable(unsigned long start, unsigned long length);

private:
    static const unsigned int FRAME_SIZE = Machine::PAGE_SIZE;

    FreeBlock *head;
};

#endif
