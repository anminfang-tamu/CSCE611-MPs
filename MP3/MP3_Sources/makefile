AS=nasm
PLATFORM=$(shell uname)
$(info $(PLATFORM))

ifeq ($(PLATFORM), Linux)
GCC=x86_64-linux-gnu-gcc
LD=x86_64-linux-gnu-ld
endif

ifeq ($(PLATFORM), Darwin)
GCC=x86_64-elf-gcc
LD=x86_64-elf-ld
endif

GCC_OPTIONS = -m32 -nostdlib -fno-builtin -nostartfiles -nodefaultlibs -fno-exceptions -fno-rtti -fno-stack-protector -fleading-underscore -fno-asynchronous-unwind-tables -fno-pie

all: kernel.bin

clean:
	rm -f *.o *.bin

run:
	qemu-system-x86_64 -kernel kernel.bin -serial stdio

debug:
	qemu-system-x86_64 -s -S -kernel kernel.bin

# ==== KERNEL ENTRY POINT ====

start.o: start.asm gdt_low.asm idt_low.asm irq_low.asm
	$(AS) -f elf -o start.o start.asm

# ==== UTILITIES ====

utils.o: utils.C utils.H
	$(GCC) $(GCC_OPTIONS) -c -o utils.o utils.C

assert.o: assert.C assert.H
	$(GCC) $(GCC_OPTIONS) -c -o assert.o assert.C

# ==== VARIOUS LOW-LEVEL STUFF =====

gdt.o: gdt.C gdt.H
	$(GCC) $(GCC_OPTIONS) -c -o gdt.o gdt.C

machine.o: machine.C machine.H
	$(GCC) $(GCC_OPTIONS) -c -o machine.o machine.C

machine_low.o: machine_low.asm machine_low.H
	nasm -f elf -o machine_low.o machine_low.asm

# ==== EXCEPTIONS AND INTERRUPTS =====

idt.o: idt.C idt.H
	$(GCC) $(GCC_OPTIONS) -c -o idt.o idt.C

irq.o: irq.C irq.H
	$(GCC) $(GCC_OPTIONS) -c -o irq.o irq.C

exceptions.o: exceptions.C exceptions.H
	$(GCC) $(GCC_OPTIONS) -c -o exceptions.o exceptions.C

interrupts.o: interrupts.C interrupts.H
	$(GCC) $(GCC_OPTIONS) -c -o interrupts.o interrupts.C

# ==== DEVICES =====

console.o: console.C console.H
	$(GCC) $(GCC_OPTIONS) -c -o console.o console.C

simple_timer.o: simple_timer.C simple_timer.H
	$(GCC) $(GCC_OPTIONS) -c -o simple_timer.o simple_timer.C

# ==== MEMORY =====

paging_low.o: paging_low.asm paging_low.H
	nasm -f elf -o paging_low.o paging_low.asm

page_table.o: page_table.C page_table.H paging_low.H
	$(GCC) $(GCC_OPTIONS) -c -o page_table.o page_table.C

# cont_frame_pool.o: cont_frame_pool.C cont_frame_pool.H
# 	$(GCC) $(GCC_OPTIONS) -c -o cont_frame_pool.o cont_frame_pool.C

# ==== KERNEL MAIN FILE =====

kernel.o: kernel.C console.H simple_timer.H page_table.H
	$(GCC) $(GCC_OPTIONS) -c -o kernel.o kernel.C

kernel.bin: start.o utils.o kernel.o assert.o console.o gdt.o idt.o irq.o exceptions.o \
   interrupts.o simple_timer.o paging_low.o page_table.o cont_frame_pool.o machine.o \
   machine_low.o 
	$(LD) -melf_i386 -T linker.ld -o kernel.bin start.o utils.o kernel.o assert.o console.o \
   gdt.o idt.o irq.o exceptions.o \
   interrupts.o simple_timer.o paging_low.o page_table.o cont_frame_pool.o machine.o \
   machine_low.o
