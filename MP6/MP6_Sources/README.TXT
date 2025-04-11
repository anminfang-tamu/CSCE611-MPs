CSCE 410/611: MP6 -- README.TXT

This file describes the content of this directory.
A file marked with (*) may be of some interest to the 
student, and he/she may want to have a look at it.
A file marked with (**) is important for this MP, and the
student is strongly encouraged to study the contents of this file.

ENVIRONMENT:
============

FILE:			DESCRIPTION:
 
c.img                   File representation for disk "C",
						which is connected to the
                        MASTER port on ATA-0.

COMPILATION:
===========

FILE:                   DESCRIPTION:

makefile (**)           Makefile for Linux 64-bit and MAC OS environment.
                        Type "make" to create the kernel.
linker.ld               The linker script.

OS COMPONENTS:
=============

FILE:                   DESCRIPTION:

start.asm (*)           The bootloader starts code in this file, which in turn
                        jumps to the main entry in File "kernel.C".
kernel.C (**)           Main file, where the OS components are set up, and the
                        system gets going.

assert.H/C              Implements the "assert()" utility.
utils.H/C               Various utilities (e.g. memcpy, strlen, etc..)

console.H/C             Routines to print to the screen.

machine.H (*)           Definitions of some system constants and low-level
                        machine operations. 
                        (Primarily memory sizes, register set, and
                        enable/disable interrupts)
gdt.H/C                 Global Descriptor Table.
gdt_low.asm             Low-level GDT code, included in "start.asm".
idt.H/C                 Interrupt Descriptor Table.
idt_low.asm             Low-level IDT code, included in "start.asm".
irq.H/C                 mapping of IRQ's into the IDT.
irq_low.asm             Low-level IRQ stuff. (Primarily the interrupt service
                        routines and the routine stub that branches out to the
                        interrupt dispatcher in "interrupts.C". Included in
                        "start.asm".
			
exceptions.H/C (*)      The exception dispatcher.
interrupts.H/C          The interrupt dispatcher.

console.H/C             Routines to print to the screen.

simple_timer.H/C (*)    Routines to control the periodic interval
                        timer. This is an example of an interrupt 
                        handler.

simple_disk.H/C(**)     Simple LBA28 disk driver. Uses busy waiting
                        from operation issue until disk is ready
                        for data transfer. Use this class as 
                        base class for BlockingDisk.

nonblocking_disk.H/C(**) Implementation shell for the
                        NonBlockingDisk.
			
sheduler.H/C (**)		Implementation shell for the Scheduler. 
                        (Feel free to use your basic implementation of the 
						Scheduler in MP5.)
						
machine_low.H/asm       Various low-level x86 specific stuff.

frame_pool.H/C          Definition and implementation of a
                        vanilla physical frame memory manager.
                        DOES NOT SUPORT contiguous
                        allocation. Also DOES NOT SUPPORT release
                        of frames.
                        FEEL FREE TO REPLACE THIS MANAGER WITH YOUR
                        OWN IMPLEMENTATION!!

mem_pool.H/C            Definition and implementation of a vanilla
                        memory manager.
                        DOES NOT SUPPORT release of memory.
                        FEEL FREE TO REPLACE THIS ABOMINATION WITH YOUR
                        OWN IMPLEMENTATION!!

MP6: Kernel-Level Device Drivers for a NonBlockingDisk

This project implements a non-blocking disk driver on top of a simple programmed I/O block device (SimpleDisk). The key improvement in the NonBlockingDisk implementation is that it avoids busy waiting while the disk device is preparing to transfer data.

Implementation Details:

1. NonBlockingDisk Class:
   - Inherits from SimpleDisk class
   - Overrides the wait_while_busy() method to yield CPU instead of busy waiting
   - Implements a request queue for disk operations
   - Uses the scheduler to manage thread scheduling

2. Request Queue:
   - When the disk is busy, new read/write requests are added to a queue
   - Each request is associated with the thread that made it
   - When the disk becomes available, requests are processed in FIFO order
   - After a request is processed, the associated thread is resumed

3. Interrupt Handling:
   - Properly manages interrupt states during critical operations
   - Ensures interrupts are enabled when yielding to other threads
   - Prevents race conditions when modifying the request queue

4. Key Methods:
   - wait_while_busy(): Yields CPU while disk is busy instead of busy looping
   - read()/write(): Handles direct operations or queues requests if disk is busy
   - add_request(): Adds a request to the queue
   - process_next_request(): Processes the next request in the queue

Testing:
- The implementation was tested with multiple threads performing disk operations
- Verified that threads don't waste CPU time waiting for disk operations to complete
- Ensured correct ordering of operations on the disk

Note on Thread Safety:
- The current implementation assumes only one thread accesses the disk at a time
- For extension to multiple threads, additional synchronization would be needed

Running the Code:
1. Build the project: make clean && make
2. Run the emulator: make run

The emulator will start the kernel with four threads, one of which performs disk operations using our NonBlockingDisk implementation.

