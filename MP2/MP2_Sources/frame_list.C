#include "frame_list.H"
#include "cont_frame_pool.H"
#include "utils.H"
#include "console.H"

static FrameList::FreeBlock freeblock_storage[128];
static int freeblock_storage_pos = 0;

// equals to new FreeBlock()
static FrameList::FreeBlock *get_free_block(const FrameList::FreeBlock &src)
{
    if (freeblock_storage_pos >= 128)
    {
        Console::puts("Error: out of freeblock_storage!\n");
        return nullptr;
    }
    freeblock_storage[freeblock_storage_pos] = src;
    FrameList::FreeBlock *ret = &freeblock_storage[freeblock_storage_pos];
    freeblock_storage_pos++;
    return ret;
}

void FrameList::init_managed_region(unsigned long start,
                                    unsigned long length,
                                    void *storage)
{
    head = (FreeBlock *)storage;
    head->start = start;
    head->length = length;
    head->next = nullptr;
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
    Console::puts("Failed to allocate frames\n");
    return 0;
}

void FrameList::release(unsigned long start, unsigned long length)
{
    FreeBlock temp;
    temp.start = start;
    temp.length = length;
    temp.next = nullptr;

    // Step 1: find the position to insert
    FreeBlock *curr = head;
    FreeBlock *prev = nullptr;
    while (curr && (curr->start < start))
    {
        prev = curr;
        curr = curr->next;
    }

    // Step 2: try to merge with prev
    if (prev && (prev->start + prev->length == start))
    {
        prev->length += length; // merged
        // Step 3: try to merge with curr
        if (curr && (prev->start + prev->length == curr->start))
        {
            prev->length += curr->length;
            prev->next = curr->next;
        }
        return;
    }

    // Step 4: try to merge with curr
    if (curr && (start + length == curr->start))
    {
        // Step 5: build a block merged with curr
        temp.length += curr->length;
        temp.next = curr->next;
        // Step 6: insert 'temp' in place of 'curr'
        if (prev)
            prev->next = get_free_block(temp);
        else
            head = get_free_block(temp);
        return;
    }

    // Step 7: otherwise just insert a new block
    FreeBlock *newb = get_free_block(temp);
    if (prev)
    {
        prev->next = newb;
        newb->next = curr;
    }
    else
    {
        newb->next = head;
        head = newb;
    }
}

inline unsigned long min(unsigned long a, unsigned long b)
{
    return (a < b) ? a : b;
}

void FrameList::mark_unavailable(unsigned long start, unsigned long length)
{
    unsigned long block_start = allocate(length);
    (void)block_start;
}
