/*
     File        : file.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File class, with support for
                   sequential read/write operations.
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
#include "file.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR */
/*--------------------------------------------------------------------------*/

File::File(FileSystem *_fs, int _id)
{
    Console::puts("Opening file.\n");

    fs = _fs;
    current_position = 0;
    current_block = 0;

    // Get file inode
    inode = fs->LookupFile(_id);
    if (inode == nullptr)
    {
        Console::puts("Error: File not found.\n");
        assert(false);
    }

    // Load first block if exists
    unsigned int first_block_no = inode->GetBlockNo(0);
    if (first_block_no != 0)
    {
        fs->disk->read(first_block_no, block_cache);
    }
    else
    {
        memset(block_cache, 0, SimpleDisk::BLOCK_SIZE);
    }
}

File::~File()
{
    Console::puts("Closing file.\n");

    // Write cached block if allocated
    unsigned int current_block_no = inode->GetBlockNo(current_block);
    if (current_block_no != 0)
    {
        fs->disk->write(current_block_no, block_cache);
    }
}

/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/

int File::Read(unsigned int _n, char *_buf)
{
    Console::puts("reading from file\n");

    // Check if we're at the end of the file
    if (current_position >= inode->file_size)
    {
        return 0; // Nothing to read
    }

    // Calculate how many bytes we can actually read
    unsigned int bytes_to_read = _n;
    if (current_position + bytes_to_read > inode->file_size)
    {
        bytes_to_read = inode->file_size - current_position;
    }

    unsigned int bytes_read = 0;
    unsigned int buf_position = 0;

    while (bytes_read < bytes_to_read)
    {
        // Calculate position within the current block
        unsigned int block_index = current_position / SimpleDisk::BLOCK_SIZE;
        unsigned int block_offset = current_position % SimpleDisk::BLOCK_SIZE;

        // Check if we need to load a new block
        if (block_index != current_block)
        {
            // Write the current block back to disk if it has been modified
            unsigned int current_block_no = inode->GetBlockNo(current_block);
            if (current_block_no != 0)
            {
                fs->disk->write(current_block_no, block_cache);
            }

            // Load the new block
            current_block = block_index;
            unsigned int new_block_no = inode->GetBlockNo(current_block);

            if (new_block_no != 0)
            {
                // Load the block from disk
                fs->disk->read(new_block_no, block_cache);
            }
            else
            {
                // This block hasn't been allocated, which shouldn't happen during reading
                Console::puts("Warning: Trying to read from non-allocated block\n");
                memset(block_cache, 0, SimpleDisk::BLOCK_SIZE);
                return bytes_read; // Return what we've read so far
            }
        }

        // Calculate how many bytes we can read from this block
        unsigned int bytes_to_read_from_block = SimpleDisk::BLOCK_SIZE - block_offset;
        if (bytes_to_read_from_block > (bytes_to_read - bytes_read))
        {
            bytes_to_read_from_block = bytes_to_read - bytes_read;
        }

        // Copy data from the cache to the buffer
        memcpy(_buf + buf_position, block_cache + block_offset, bytes_to_read_from_block);

        // Update positions
        current_position += bytes_to_read_from_block;
        bytes_read += bytes_to_read_from_block;
        buf_position += bytes_to_read_from_block;
    }

    return bytes_read;
}

int File::Write(unsigned int _n, const char *_buf)
{
    Console::puts("writing to file\n");

    unsigned int bytes_to_write = _n;
    unsigned int bytes_written = 0;
    unsigned int buf_position = 0;

    // Max file size is 64KB (128 blocks)
    unsigned int max_file_size = (Inode::MAX_DIRECT_BLOCKS + Inode::BLOCKS_PER_INDIRECT) * SimpleDisk::BLOCK_SIZE;
    if (current_position + bytes_to_write > max_file_size)
    {
        bytes_to_write = max_file_size - current_position;
    }

    if (bytes_to_write == 0)
    {
        return 0; // Nothing to write
    }

    while (bytes_written < bytes_to_write)
    {
        // Calculate position within the current block
        unsigned int block_index = current_position / SimpleDisk::BLOCK_SIZE;
        unsigned int block_offset = current_position % SimpleDisk::BLOCK_SIZE;

        // Check if we need to load a new block
        if (block_index != current_block)
        {
            // Write the current block back to disk if it has been modified
            unsigned int current_block_no = inode->GetBlockNo(current_block);
            if (current_block_no != 0)
            {
                fs->disk->write(current_block_no, block_cache);
            }

            // Update our current block tracker
            current_block = block_index;

            // Check if the new block exists
            unsigned int new_block_no = inode->GetBlockNo(current_block);
            if (new_block_no != 0)
            {
                // Block exists, read it in
                fs->disk->read(new_block_no, block_cache);
            }
            else
            {
                // Allocate a new block
                if (!inode->AllocateBlock(current_block))
                {
                    // If we couldn't allocate a block, stop writing
                    Console::puts("Failed to allocate new block when writing\n");
                    break;
                }

                // Get the new block number after allocation
                new_block_no = inode->GetBlockNo(current_block);
                if (new_block_no == 0)
                {
                    Console::puts("Block allocation error: block number still 0\n");
                    break;
                }

                // Clear the new block
                memset(block_cache, 0, SimpleDisk::BLOCK_SIZE);
            }
        }

        // Calculate how many bytes we can write to this block
        unsigned int bytes_to_write_to_block = SimpleDisk::BLOCK_SIZE - block_offset;
        if (bytes_to_write_to_block > (bytes_to_write - bytes_written))
        {
            bytes_to_write_to_block = bytes_to_write - bytes_written;
        }

        // Copy data from the buffer to the cache
        memcpy(block_cache + block_offset, _buf + buf_position, bytes_to_write_to_block);

        // Update positions
        current_position += bytes_to_write_to_block;
        bytes_written += bytes_to_write_to_block;
        buf_position += bytes_to_write_to_block;
    }

    // Update the file size if we've written past the end
    if (current_position > inode->file_size)
    {
        inode->file_size = current_position;
    }

    // Make sure to write the last block before returning
    unsigned int last_block_no = inode->GetBlockNo(current_block);
    if (last_block_no != 0)
    {
        fs->disk->write(last_block_no, block_cache);
    }

    return bytes_written;
}

void File::Reset()
{
    Console::puts("resetting file\n");

    // Check if we need to write the current block back to disk
    unsigned int current_block_no = inode->GetBlockNo(current_block);
    if (current_block_no != 0)
    {
        fs->disk->write(current_block_no, block_cache);
    }

    // Set the current position to the beginning of the file
    current_position = 0;
    current_block = 0;

    // Load the first block into the cache (if file has any blocks)
    unsigned int first_block_no = inode->GetBlockNo(0);
    if (first_block_no != 0)
    {
        fs->disk->read(first_block_no, block_cache);
    }
    else
    {
        // Clear the cache for empty files
        memset(block_cache, 0, SimpleDisk::BLOCK_SIZE);
    }
}

bool File::EoF()
{
    Console::puts("checking for EoF\n");

    // Check if the current position is at or beyond the end of the file
    return current_position >= inode->file_size;
}
