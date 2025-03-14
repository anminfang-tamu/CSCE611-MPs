/*
 File: vm_pool.C

 Author:
 Date  : 2024/09/20

 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "vm_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "page_table.H"

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
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

VMPool::VMPool(unsigned long _base_address,
               unsigned long _size,
               ContFramePool *_frame_pool,
               PageTable *_page_table) : base_address(_base_address), size(_size), frame_pool(_frame_pool), page_table(_page_table)
{
    free_regions[0] = {base_address, size};
    num_free_regions = 1;
    num_allocated_regions = 0;

    page_table->register_pool(this);
    Console::puts("Constructed VMPool object.\n");
}

unsigned long VMPool::allocate(unsigned long _size)
{
    unsigned long alloc_size = ((_size + PageTable::PAGE_SIZE - 1) / PageTable::PAGE_SIZE) * PageTable::PAGE_SIZE;

    for (unsigned int i = 0; i < num_free_regions; i++)
    {
        if (free_regions[i].size >= alloc_size)
        {

            unsigned long alloc_start = free_regions[i].start;

            assert(num_allocated_regions < MAX_REGIONS);
            allocated_regions[num_allocated_regions].start = alloc_start;
            allocated_regions[num_allocated_regions].size = alloc_size;
            num_allocated_regions++;

            free_regions[i].start += alloc_size;
            free_regions[i].size -= alloc_size;

            if (free_regions[i].size == 0)
            {
                for (unsigned int j = i; j < num_free_regions - 1; j++)
                {
                    free_regions[j] = free_regions[j + 1];
                }
                num_free_regions--;
            }

            Console::puts("Allocated region of memory: allocated_regions.size() = ");
            Console::puti(num_allocated_regions);
            Console::puts("\n");
            return alloc_start;
        }
    }

    Console::puts("Allocated region of memory: allocated_regions.size() = ");
    return 0;
}

void VMPool::release(unsigned long _start_address)
{
    for (unsigned int i = 0; i < num_allocated_regions; i++)
    {
        unsigned long num_pages = allocated_regions[i].size / PageTable::PAGE_SIZE;
        unsigned long page_no = _start_address / PageTable::PAGE_SIZE;

        for (unsigned long j = 0; j < num_pages; j++)
        {
            page_table->free_page(page_no + j);
        }

        // Add the released region to the free regions
        assert(num_free_regions < MAX_REGIONS);
        free_regions[num_free_regions] = allocated_regions[i];
        num_free_regions++;

        for (unsigned int k = i; k < num_allocated_regions - 1; k++)
        {
            allocated_regions[k] = allocated_regions[k + 1];
        }
        num_allocated_regions--;

        Console::puts("Released region of memory.\n");
        return;
    }
    Console::puts("Released region of memory.\n");
}

bool VMPool::is_legitimate(unsigned long _address)
{
    for (unsigned int i = 0; i < num_allocated_regions; i++)
    {
        unsigned long region_start = allocated_regions[i].start;
        unsigned long region_end = region_start + allocated_regions[i].size;

        if (_address >= region_start && _address < region_end)
        {
            return true;
        }
    }
    Console::puts("Checked whether address is part of an allocated region.\n");
    return false;
}
