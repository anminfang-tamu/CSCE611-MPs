/*
     File        : file_system.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File System class.
                   Has support for numerical file identifiers.
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file_system.H"

/*--------------------------------------------------------------------------*/
/* CLASS Inode */
/*--------------------------------------------------------------------------*/

/* You may need to add a few functions, for example to help read and store
   inodes from and to disk. */

unsigned int Inode::GetBlockNo(unsigned int index)
{
    // Direct block access
    if (index < MAX_DIRECT_BLOCKS)
    {
        return direct_blocks[index];
    }

    // Indirect block access
    if (index < MAX_DIRECT_BLOCKS + BLOCKS_PER_INDIRECT)
    {
        if (indirect_block == 0)
        {
            return 0;
        }

        unsigned char block_buffer[SimpleDisk::BLOCK_SIZE];
        fs->disk->read(indirect_block, block_buffer);
        unsigned int *block_ptrs = (unsigned int *)block_buffer;
        return block_ptrs[index - MAX_DIRECT_BLOCKS];
    }

    return 0;
}

bool Inode::AllocateBlock(unsigned int index)
{
    // Check if the index is valid
    if (index >= MAX_DIRECT_BLOCKS + BLOCKS_PER_INDIRECT)
    {
        return false; // Index out of range
    }

    // Get a free block from the file system
    int block_no = fs->GetFreeBlock();
    if (block_no < 0)
    {
        return false; // No free blocks available
    }

    // If index is less than MAX_DIRECT_BLOCKS, allocate a direct block
    if (index < MAX_DIRECT_BLOCKS)
    {
        direct_blocks[index] = block_no;
    }
    else
    {
        // We need to allocate or update the indirect block
        if (indirect_block == 0)
        {
            // Allocate a block for the indirect block
            int indirect_block_no = fs->GetFreeBlock();
            if (indirect_block_no < 0)
            {
                // Release the previously allocated block
                fs->free_blocks[block_no] = 1; // Mark as free
                return false;                  // No free blocks available
            }

            indirect_block = indirect_block_no;

            // Initialize the indirect block with zeros
            unsigned char block_buffer[SimpleDisk::BLOCK_SIZE];
            memset(block_buffer, 0, SimpleDisk::BLOCK_SIZE);
            fs->disk->write(indirect_block, block_buffer);

            // Mark the indirect block as used
            fs->free_blocks[indirect_block] = 0;
        }

        // Read the indirect block from disk
        unsigned char block_buffer[SimpleDisk::BLOCK_SIZE];
        fs->disk->read(indirect_block, block_buffer);

        // Update the indirect block
        unsigned int *block_ptrs = (unsigned int *)block_buffer;
        block_ptrs[index - MAX_DIRECT_BLOCKS] = block_no;

        // Write the indirect block back to disk
        fs->disk->write(indirect_block, block_buffer);
    }

    // Mark the block as used in the free block list
    fs->free_blocks[block_no] = 0;

    // Update number of blocks allocated
    num_blocks_allocated++;

    // Initialize the block with zeros
    unsigned char block_buffer[SimpleDisk::BLOCK_SIZE];
    memset(block_buffer, 0, SimpleDisk::BLOCK_SIZE);
    fs->disk->write(block_no, block_buffer);

    return true;
}

void Inode::FreeBlocks()
{
    // Free all direct blocks
    for (unsigned int i = 0; i < MAX_DIRECT_BLOCKS; i++)
    {
        if (direct_blocks[i] != 0)
        {
            fs->free_blocks[direct_blocks[i]] = 1; // Mark as free
            direct_blocks[i] = 0;
        }
    }

    // Free indirect blocks if they exist
    if (indirect_block != 0)
    {
        // Read the indirect block from disk
        unsigned char block_buffer[SimpleDisk::BLOCK_SIZE];
        fs->disk->read(indirect_block, block_buffer);

        // Free all blocks pointed to by the indirect block
        unsigned int *block_ptrs = (unsigned int *)block_buffer;
        for (unsigned int i = 0; i < BLOCKS_PER_INDIRECT; i++)
        {
            if (block_ptrs[i] != 0)
            {
                fs->free_blocks[block_ptrs[i]] = 1; // Mark as free
            }
        }

        // Free the indirect block itself
        fs->free_blocks[indirect_block] = 1; // Mark as free
        indirect_block = 0;
    }

    // Reset the number of blocks allocated
    num_blocks_allocated = 0;
}

/*--------------------------------------------------------------------------*/
/* FileSystem Implementation */
/*--------------------------------------------------------------------------*/

FileSystem::FileSystem()
{
    Console::puts("In file system constructor.\n");

    disk = nullptr;
    size = 0;
    inodes = new Inode[MAX_INODES];

    // Initialize inodes
    for (unsigned int i = 0; i < MAX_INODES; i++)
    {
        inodes[i].id = -1;
        inodes[i].is_valid = false;
        inodes[i].file_size = 0;
        for (unsigned int j = 0; j < Inode::MAX_DIRECT_BLOCKS; j++)
        {
            inodes[i].direct_blocks[j] = 0;
        }
        inodes[i].indirect_block = 0;
        inodes[i].num_blocks_allocated = 0;
        inodes[i].fs = this;
    }

    free_blocks = nullptr;
}

FileSystem::~FileSystem()
{
    Console::puts("unmounting file system\n");
    /* Make sure that the inode list and the free list are saved. */

    if (disk != nullptr)
    {
        // Save the inodes and free block list to disk
        SaveInodes();
        SaveFreeList();

        // Free allocated memory
        delete[] free_blocks;
        delete[] inodes;
    }
}

/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/

bool FileSystem::Mount(SimpleDisk *_disk)
{
    Console::puts("mounting file system\n");
    /* Here you read the free block list and the inode list from the disk
       and initialize the class variables */

    if (_disk == nullptr)
    {
        Console::puts("Error: Disk is null\n");
        return false;
    }

    this->disk = _disk;
    this->size = disk->NaiveSize(); // Use the correct method to get the disk size
    unsigned int num_blocks = this->size / SimpleDisk::BLOCK_SIZE;
    Console::puts("Mounting disk with ");
    Console::puti(num_blocks);
    Console::puts(" blocks\n");

    // Allocate memory for our system structures
    this->inodes = new Inode[MAX_INODES];
    this->free_blocks = new unsigned char[num_blocks];

    // Calculate the maximum number of inodes that can fit in a block
    unsigned int inode_size = sizeof(Inode);
    unsigned int max_inodes_per_block = SimpleDisk::BLOCK_SIZE / inode_size;
    unsigned int safe_max_inodes = (max_inodes_per_block < MAX_INODES) ? max_inodes_per_block : MAX_INODES;

    // Read the inode list from disk (block 0)
    unsigned char block_buffer[SimpleDisk::BLOCK_SIZE];
    this->disk->read(INODES_BLOCK, block_buffer);

    // Copy only as many inodes as can fit in a block or MAX_INODES (whichever is smaller)
    unsigned int inodes_to_copy = safe_max_inodes * sizeof(Inode);
    if (inodes_to_copy > SimpleDisk::BLOCK_SIZE)
    {
        Console::puts("ERROR: Inode structure too large for disk block!\n");
        delete[] this->inodes;
        delete[] this->free_blocks;
        return false;
    }

    memcpy(this->inodes, block_buffer, inodes_to_copy);

    // Read the free block list from disk (block 1)
    this->disk->read(FREELIST_BLOCK, block_buffer);

    // Only copy as many blocks as we have or can fit in a block
    unsigned int freelist_bytes = (num_blocks < SimpleDisk::BLOCK_SIZE) ? num_blocks : SimpleDisk::BLOCK_SIZE;
    memcpy(this->free_blocks, block_buffer, freelist_bytes);

    // Verify that the disk is formatted
    bool is_formatted = false;
    for (unsigned int i = 0; i < safe_max_inodes; i++)
    {
        if (this->inodes[i].id != -1 && this->inodes[i].is_valid)
        {
            is_formatted = true;
            break;
        }
    }

    if (!is_formatted)
    {
        Console::puts("WARNING: Disk appears to be unformatted or inode read failed\n");
    }

    // Set filesystem pointer for all inodes
    for (unsigned int i = 0; i < MAX_INODES; i++)
    {
        inodes[i].fs = this;
    }

    Console::puts("mounting completed successfully\n");
    return true;
}

bool FileSystem::Format(SimpleDisk *_disk, unsigned int _size)
{ // static!
    Console::puts("formatting disk\n");
    /* Here you populate the disk with an initialized (probably empty) inode list
       and a free list. Make sure that blocks used for the inodes and for the free list
       are marked as used, otherwise they may get overwritten. */

    if (_disk == nullptr)
    {
        Console::puts("Error: Disk is null\n");
        return false;
    }

    // Calculate the number of blocks in our file system
    unsigned int num_blocks = _size / SimpleDisk::BLOCK_SIZE;
    Console::puts("Formatting with ");
    Console::puti(num_blocks);
    Console::puts(" blocks\n");

    // Check if the Inode structure will fit in a single block
    unsigned int inode_size = sizeof(Inode);
    unsigned int max_inodes_per_block = SimpleDisk::BLOCK_SIZE / inode_size;

    Console::puts("Inode size: ");
    Console::puti(inode_size);
    Console::puts(" bytes, max inodes per block: ");
    Console::puti(max_inodes_per_block);
    Console::puts("\n");

    // Ensure MAX_INODES doesn't exceed what can fit in a block
    unsigned int safe_max_inodes = (max_inodes_per_block < MAX_INODES) ? max_inodes_per_block : MAX_INODES;

    // Create temporary structures to initialize the disk
    Inode *temp_inodes = new Inode[safe_max_inodes];
    unsigned char *temp_free_blocks = new unsigned char[num_blocks];

    // Initialize all inodes to invalid
    for (unsigned int i = 0; i < safe_max_inodes; i++)
    {
        temp_inodes[i].id = -1;
        temp_inodes[i].is_valid = false;
        temp_inodes[i].file_size = 0;
        for (unsigned int j = 0; j < Inode::MAX_DIRECT_BLOCKS; j++)
        {
            temp_inodes[i].direct_blocks[j] = 0;
        }
        temp_inodes[i].indirect_block = 0;
        temp_inodes[i].num_blocks_allocated = 0;
    }

    // Initialize the free block list
    // Mark all blocks as free (1) initially
    for (unsigned int i = 0; i < num_blocks; i++)
    {
        temp_free_blocks[i] = 1; // 1 means free, 0 means used
    }

    // Mark the first two blocks (inode block and free list block) as used
    temp_free_blocks[INODES_BLOCK] = 0;
    temp_free_blocks[FREELIST_BLOCK] = 0;

    // Write the inode list to disk (block 0)
    unsigned char block_buffer[SimpleDisk::BLOCK_SIZE];
    memset(block_buffer, 0, SimpleDisk::BLOCK_SIZE); // Clear the buffer first

    // Only copy as many inodes as can fit in a block
    unsigned int inodes_bytes = sizeof(Inode) * safe_max_inodes;
    if (inodes_bytes > SimpleDisk::BLOCK_SIZE)
    {
        Console::puts("ERROR: Inode structure too large for disk block!\n");
        delete[] temp_inodes;
        delete[] temp_free_blocks;
        return false;
    }

    memcpy(block_buffer, temp_inodes, inodes_bytes);
    _disk->write(INODES_BLOCK, block_buffer);

    // Write the free block list to disk (block 1)
    memset(block_buffer, 0, SimpleDisk::BLOCK_SIZE); // Clear the buffer first

    // Make sure we don't exceed the block size for the free list
    unsigned int freelist_bytes = (num_blocks < SimpleDisk::BLOCK_SIZE) ? num_blocks : SimpleDisk::BLOCK_SIZE;
    memcpy(block_buffer, temp_free_blocks, freelist_bytes);
    _disk->write(FREELIST_BLOCK, block_buffer);

    // Clean up
    delete[] temp_inodes;
    delete[] temp_free_blocks;

    Console::puts("formatting completed successfully\n");
    return true;
}

Inode *FileSystem::LookupFile(int _file_id)
{
    Console::puts("looking up file with id = ");
    Console::puti(_file_id);
    Console::puts("\n");
    /* Here you go through the inode list to find the file. */

    for (unsigned int i = 0; i < MAX_INODES; i++)
    {
        if (inodes[i].is_valid && inodes[i].id == _file_id)
        {
            return &inodes[i];
        }
    }

    return nullptr; // File not found
}

bool FileSystem::CreateFile(int _file_id)
{
    Console::puts("creating file with id:");
    Console::puti(_file_id);
    Console::puts("\n");
    /* Here you check if the file exists already. If so, throw an error.
       Then get yourself a free inode and initialize all the data needed for the
       new file. After this function there will be a new file on disk. */

    // Check if file already exists
    if (LookupFile(_file_id) != nullptr)
    {
        Console::puts("Error: File already exists.\n");
        return false;
    }

    // Get a free inode
    short inode_index = GetFreeInode();
    if (inode_index < 0)
    {
        Console::puts("Error: No free inodes available.\n");
        return false;
    }

    // Initialize the inode
    inodes[inode_index].id = _file_id;
    inodes[inode_index].is_valid = true;
    inodes[inode_index].file_size = 0;
    for (unsigned int i = 0; i < Inode::MAX_DIRECT_BLOCKS; i++)
    {
        inodes[inode_index].direct_blocks[i] = 0;
    }
    inodes[inode_index].indirect_block = 0;
    inodes[inode_index].num_blocks_allocated = 0;
    inodes[inode_index].fs = this;

    // Allocate the first block for the file
    if (!inodes[inode_index].AllocateBlock(0))
    {
        // If we couldn't allocate a block, mark the inode as invalid
        inodes[inode_index].is_valid = false;
        inodes[inode_index].id = -1;
        Console::puts("Error: Could not allocate first block.\n");
        return false;
    }

    // Save the changes to disk
    SaveInodes();
    SaveFreeList();

    return true;
}

bool FileSystem::DeleteFile(int _file_id)
{
    Console::puts("deleting file with id:");
    Console::puti(_file_id);
    Console::puts("\n");
    /* First, check if the file exists. If not, throw an error.
       Then free all blocks that belong to the file and delete/invalidate
       (depending on your implementation of the inode list) the inode. */

    // Find the file's inode
    Inode *inode = LookupFile(_file_id);
    if (inode == nullptr)
    {
        Console::puts("Error: File does not exist.\n");
        return false;
    }

    // Free all blocks used by the file
    inode->FreeBlocks();

    // Invalidate the inode
    inode->is_valid = false;
    inode->id = -1;
    inode->file_size = 0;

    // Save changes to disk
    SaveInodes();
    SaveFreeList();

    return true;
}

/*--------------------------------------------------------------------------*/
/* HELPER FUNCTIONS */
/*--------------------------------------------------------------------------*/

short FileSystem::GetFreeInode()
{
    for (unsigned int i = 0; i < MAX_INODES; i++)
    {
        if (!inodes[i].is_valid)
        {
            return i;
        }
    }
    return -1; // No free inodes
}

int FileSystem::GetFreeBlock()
{
    // Calculate number of blocks
    unsigned int num_blocks = size / SimpleDisk::BLOCK_SIZE;

    // Start from the first data block (skip inode and free list blocks)
    for (unsigned int i = FIRST_DATA_BLOCK; i < num_blocks; i++)
    {
        if (free_blocks[i] == 1)
        { // If block is free
            return i;
        }
    }
    return -1; // No free blocks
}

void FileSystem::SaveInodes()
{
    unsigned char block_buffer[SimpleDisk::BLOCK_SIZE];
    memset(block_buffer, 0, SimpleDisk::BLOCK_SIZE); // Clear the buffer first
    memcpy(block_buffer, inodes, sizeof(Inode) * MAX_INODES);
    disk->write(INODES_BLOCK, block_buffer);
}

void FileSystem::LoadInodes()
{
    unsigned char block_buffer[SimpleDisk::BLOCK_SIZE];
    disk->read(INODES_BLOCK, block_buffer);
    memcpy(inodes, block_buffer, sizeof(Inode) * MAX_INODES);

    // Make sure all inodes have a reference to this file system
    for (unsigned int i = 0; i < MAX_INODES; i++)
    {
        inodes[i].fs = this;
    }
}

void FileSystem::SaveFreeList()
{
    unsigned char block_buffer[SimpleDisk::BLOCK_SIZE];
    memset(block_buffer, 0, SimpleDisk::BLOCK_SIZE); // Clear the buffer first
    unsigned int num_blocks = size / SimpleDisk::BLOCK_SIZE;

    // Make sure we don't exceed the block size for the free list
    unsigned int freelist_bytes = (num_blocks < SimpleDisk::BLOCK_SIZE) ? num_blocks : SimpleDisk::BLOCK_SIZE;
    memcpy(block_buffer, free_blocks, freelist_bytes);
    disk->write(FREELIST_BLOCK, block_buffer);
}

void FileSystem::LoadFreeList()
{
    unsigned char block_buffer[SimpleDisk::BLOCK_SIZE];
    disk->read(FREELIST_BLOCK, block_buffer);
    unsigned int num_blocks = size / SimpleDisk::BLOCK_SIZE;

    // Make sure we don't exceed the block size for the free list
    unsigned int freelist_bytes = (num_blocks < SimpleDisk::BLOCK_SIZE) ? num_blocks : SimpleDisk::BLOCK_SIZE;
    memcpy(free_blocks, block_buffer, freelist_bytes);
}
