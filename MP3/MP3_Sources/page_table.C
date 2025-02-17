#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"
#include "cont_frame_pool.H"

PageTable *PageTable::current_page_table = nullptr;
unsigned int PageTable::paging_enabled = 0;
ContFramePool *PageTable::kernel_mem_pool = nullptr;
ContFramePool *PageTable::process_mem_pool = nullptr;
unsigned long PageTable::shared_size = 0;

void PageTable::init_paging(ContFramePool *_kernel_mem_pool,
                            ContFramePool *_process_mem_pool,
                            const unsigned long _shared_size)
{
   kernel_mem_pool = _kernel_mem_pool;
   process_mem_pool = _process_mem_pool;
   shared_size = _shared_size;
}

PageTable::PageTable()
{
   // step 1:Allocate a frame for the page directory
   page_directory = (unsigned long *)kernel_mem_pool->get_frames(1);

   // step 2: Initialize all entries to 0 (0: not present, 1: present)
   for (unsigned int i = 0; i < ENTRIES_PER_PAGE; i++)
   {
      page_directory[i] = 0;
   }

   // step 3: Allocate a frame for the first page table (mapping first 4MB)
   unsigned long *first_page_table = (unsigned long *)kernel_mem_pool->get_frames(1);

   // step 4: Set up the first page directory entry to point to our page table
   // 7 = xxxxx...111, make PDE present, read/write, user/supervisor
   page_directory[0] = ((unsigned long)first_page_table) | 7;

   // step 5: Initialize the page table entries for the first 4MB
   for (unsigned int i = 0; i < ENTRIES_PER_PAGE; i++)
   {
      unsigned long physical_addr = i * PAGE_SIZE;
      first_page_table[i] = physical_addr | 7;
   }

   Console::puts("============== Page Table created. ==============\n");
}

void PageTable::load()
{
   assert(false);
   Console::puts("Loaded page table\n");
}

void PageTable::enable_paging()
{
   assert(false);
   Console::puts("Enabled paging\n");
}

void PageTable::handle_fault(REGS *_r)
{
   assert(false);
   Console::puts("handled page fault\n");
}
