; bkerndev - Bran's Kernel Development Tutorial
; By:   Brandon F. (friesenb@gmail.com)
; Desc: Kernel entry point, stack, and Interrupt Service Routines.
;
; Notes: No warranty expressed or implied. Use at own risk.
;
; This is the kernel's entry point. We could either call main here,
; or we can use this to setup the stack or other nice stuff, like
; perhaps setting up the GDT and segments. Please note that interrupts
; are disabled at this point: More on interrupts later!
[BITS 32]
global start
start:
    mov esp, _sys_stack     ; This points the stack to our new stack area
    jmp stublet

; This part MUST be 4byte aligned, so we solve that issue using 'ALIGN 4'
ALIGN 4
mboot:
    ; Multiboot macros to make a few lines later more readable
    MULTIBOOT_PAGE_ALIGN	equ 1<<0
    MULTIBOOT_MEMORY_INFO	equ 1<<1
    MULTIBOOT_AOUT_KLUDGE	equ 1<<16
    MULTIBOOT_HEADER_MAGIC	equ 0x1BADB002
    MULTIBOOT_HEADER_FLAGS	equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_AOUT_KLUDGE
    MULTIBOOT_CHECKSUM	equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)
    EXTERN code, bss, end

    ; This is the GRUB Multiboot header. A boot signature
    dd MULTIBOOT_HEADER_MAGIC
    dd MULTIBOOT_HEADER_FLAGS
    dd MULTIBOOT_CHECKSUM
    
     ; AOUT kludge - must be physical addresses. Make a note of these:
    ; The linker script fills in the data for these ones!
    dd mboot 			; header_addr:
				; address corresponding to the multiboot header
    dd code			; load_addr:
				; physical address of the beginning of the text segment	
    dd bss			; load_end_addr:
				; physical address of the end of the data segment
				; (load_end_addr - load_addr) specifies how much data to load.
    dd end			; bss_end_addr:
				; pysical address of end of bss segment.
				; boot loader initializes this area to zero,
				; and reserves the memory
    dd start			; entry_addr:
				; physical address to which the boot loader should jump
				; to start running the OS

stublet:
	
; Initilization of static global objects. This goes through each object 
; in the ctors section of the object file, where the global constructors 
; created by C++ are put, and calls it. Normally C++ compilers add some code
; to do this, but that code is in the standard library - which we do not
; include
; See linker.ld to see where we tell the linker to put them.
    extern start_ctors, end_ctors, start_dtors, end_dtors

static_ctors_loop:
	mov ebx, start_ctors
	jmp .test
    .body:
	call [ebx]
	add ebx,4
    .test:
	cmp ebx, end_ctors
	jb .body

; Entering the kernel proper.
	extern _main
	call _main

; Deinitialization of static global objects. This goes through each object 
; in the dtors section of the object file, where the global destructors 
; created by C++ are put, and calls it. Normally C++ compilers add some code
; to do this, but that code is in the standard library - which we do not include.
; See linker.ld to see where we tell the linker to put them.

static_dtors_loop:
        mov ebx, start_dtors
        jmp .test
    .body:
        call [ebx]
        add ebx,4
    .test:
        cmp ebx, end_dtors
        jb .body
    
; Enter an endless loop here in order to stop.
	jmp $

; Set up Global Descriptor Table
%include "gdt_low.asm"

; Set up Low-level Exception Handling
%include "idt_low.asm"

; Set up Low-level Interrupt Handling
%include "irq_low.asm"

; Here is the definition of our BSS section. Right now, we'll use
; it just to store the stack. Remember that a stack actually grows
; downwards, so we declare the size of the data before declaring
; the identifier '_sys_stack'
SECTION .bss
    resb 16384               ; This reserves 16KBytes of memory here
_sys_stack:

