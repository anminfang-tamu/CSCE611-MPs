/*
     File        : file.H

     Author      : Riccardo Bettati
     Modified    : 2021/11/18

     Description : Simple File class with sequential read/write operations.

*/

#ifndef _FILE_H_
#define _FILE_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "file_system.H"

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

class Inode;
class FileSystem;

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* class  F i l e   */
/*--------------------------------------------------------------------------*/

class File
{
private:
   Inode *inode;                                      // File's inode
   FileSystem *fs;                                    // File system reference
   unsigned int current_position;                     // Current read/write position
   unsigned int current_block;                        // Current block index
   unsigned char block_cache[SimpleDisk::BLOCK_SIZE]; // Block cache

public:
   File(FileSystem *_fs, int _id);
   /* Opens file and initializes position to start */

   ~File();
   /* Closes file and writes cached data */

   int Read(unsigned int _n, char *_buf);
   /* Reads _n bytes from current position into _buf */

   int Write(unsigned int _n, const char *_buf);
   /* Writes _n bytes from _buf to current position */

   void Reset();
   /* Resets position to start of file */

   bool EoF();
   /* Returns true if at end of file */
};

#endif
