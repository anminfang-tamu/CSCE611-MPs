#ifndef _FRAME_LIST_H_
#define _FRAME_LIST_H_

#include "machine.H" // For PAGE_SIZE

class FrameList
{
private:
    static const unsigned int FRAME_SIZE = Machine::PAGE_SIZE;

    struct FreeBlock
    {
        unsigned long start;
        unsigned long length;
        FreeBlock *next;
    };

    FreeBlock *head;

public:
    void init(unsigned long start, unsigned long length);
    unsigned long allocate(unsigned long n_frames);
    void release(unsigned long start, unsigned long length);
    void mark_unavailable(unsigned long start, unsigned long length);
};

#endif
