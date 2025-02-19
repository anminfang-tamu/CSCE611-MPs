#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

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
   // Get a frame for the page directory
   unsigned long page_directory_frame = kernel_mem_pool->get_frames(1);
   page_directory = (unsigned long *)(page_directory_frame * PAGE_SIZE);

   // Zero out page directory
   for (unsigned int i = 0; i < ENTRIES_PER_PAGE; i++)
   {
      // 0X2 = U/S, R/W, Not Present
      // 0X3 = S/U, R/W, Present
      page_directory[i] = 0x2;
   }

   // Calculate how many page tables we need for the shared region (first 4MB)
   unsigned int num_shared_pt = (shared_size + PAGE_SIZE * ENTRIES_PER_PAGE - 1) / (PAGE_SIZE * ENTRIES_PER_PAGE);

   Console::puts("============== num_shared_pt: ");
   Console::puti(num_shared_pt);
   Console::puts(" ==============\n");

   // Set up page tables for shared region (first 4MB, direct-mapped)
   for (unsigned int page_directory_idx = 0; page_directory_idx < num_shared_pt; page_directory_idx++)
   {
      // Get a frame for the page table
      unsigned long page_table_frame = kernel_mem_pool->get_frames(1);
      unsigned long *page_table = (unsigned long *)(page_table_frame * PAGE_SIZE);

      // Set up the page directory entry
      page_directory[page_directory_idx] = (page_table_frame * PAGE_SIZE) | 0x3; // U/S, R/W, Present

      // Set up the page table entries for direct mapping
      for (unsigned int page_table_idx = 0; page_table_idx < ENTRIES_PER_PAGE; page_table_idx++)
      {
         unsigned long addr = (page_directory_idx * ENTRIES_PER_PAGE + page_table_idx) * PAGE_SIZE;
         if (addr < shared_size)
         {
            // Direct mapping: virtual address = physical address
            page_table[page_table_idx] = addr | 0x3; // U/S, R/W, Present
         }
         else
         {
            page_table[page_table_idx] = 0x2; // U/S, R/W, Not Present
         }
      }
   }

   Console::puts("============== Page Table created. ==============\n");
}

void PageTable::load()
{
   current_page_table = this;
   write_cr3((unsigned long)page_directory);

   Console::puts("============== Page Table loaded. ==============\n");
}

void PageTable::enable_paging()
{
   if (!paging_enabled)
   {
      unsigned long cr0 = read_cr0();
      cr0 |= 0x80000000; // Set the paging bit (PG)
      // cr0 |= 0x00000001; // Set the protection enable bit (PE)
      write_cr0(cr0);
      paging_enabled = 1;
   }

   Console::puts("============== Paging enabled. ==============\n");
}

void PageTable::handle_fault(REGS *_r)
{
   // get faulting address from CR2
   unsigned long fault_addr = read_cr2();

   // Console::puts("============== Faulting address: ");
   // Console::puti(fault_addr);
   // Console::puts(" ==============\n");

   // fault in shared region
   if (fault_addr < shared_size)
   {
      Console::puts("Page fault in shared region! This should not happen.\n");
      assert(false);
      return;
   }

   // page directory index and page table index
   unsigned long page_directory_idx = fault_addr >> 22;
   unsigned long page_table_idx = (fault_addr >> 12) & 0x3FF;

   // get page directory entry
   unsigned long page_directory_entry = current_page_table->page_directory[page_directory_idx];
   unsigned long *page_table;

   // page table not exist
   if (!(page_directory_entry & 0x1))
   {
      // get a frame for the page table
      unsigned long page_table_frame = kernel_mem_pool->get_frames(1);
      page_table = (unsigned long *)(page_table_frame * PAGE_SIZE);

      // init page table
      for (unsigned int i = 0; i < ENTRIES_PER_PAGE; i++)
      {
         page_table[i] = 0x2; // Supervisor, Read/Write, Not Present
      }

      // Console::puts("============== Page table created. ==============\n");
      // Console::puti(page_table_frame);
      // Console::puts(" ==============\n");

      // update page directory entry
      current_page_table->page_directory[page_directory_idx] = (page_table_frame * PAGE_SIZE) | 0x3;
   }
   else
   {
      page_table = (unsigned long *)(page_directory_entry & 0xFFFFF000);
   }

   // page table entry not exist
   if (!(page_table[page_table_idx] & 0x1))
   {
      // Get a frame for the page
      unsigned long frame = process_mem_pool->get_frames(1);

      // Update the page table entry
      page_table[page_table_idx] = (frame * PAGE_SIZE) | 0x3;
   }

   Console::puts("============== Page fault handled. ==============\n");
}
