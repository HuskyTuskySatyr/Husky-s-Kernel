/* Declare constants for the multiboot header. */
.set ALIGN,    1<<0             /* align loaded modules on page boundaries */
.set MEMINFO,  1<<1             /* provide memory map */
.set FLAGS,    ALIGN | MEMINFO  /* this is the Multiboot 'flag' field */
.set MAGIC,    0x1BADB002       /* 'magic number' lets bootloader find the header */
.set CHECKSUM, -(MAGIC + FLAGS) /* checksum of above, to prove we are multiboot */

/* 
Declare a multiboot header that marks the program as a kernel. These are magic
values that are documented in the multiboot standard. The bootloader will
search for this signature in the first 8 KiB of the kernel file, aligned at a
32-bit boundary. The signature is in its own section so the header can be
forced to be within the first 8 KiB of the kernel file.
*/
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

/*
The multiboot standard does not define the value of the stack pointer register
(esp) and it is up to the kernel to provide a stack. This allocates room for a
small stack by creating a symbol at the bottom of it, then allocating 16384
bytes for it, and finally creating a symbol at the top. The stack grows
downwards on x86. The stack is in its own section so it can be marked nobits,
which means the kernel file is smaller because it does not contain an
uninitialized stack. The stack on x86 must be 16-byte aligned according to the
System V ABI standard and de-facto extensions. The compiler will assume the
stack is properly aligned and failure to align the stack will result in
undefined behavior.
*/
.section .bss
.align 16
stack_bottom:
.skip 16384 # 16 KiB
stack_top:

/*
The linker script specifies _start as the entry point to the kernel and the
bootloader will jump to this position once the kernel has been loaded. It
doesn't make sense to return from this function as the bootloader is gone.
*/
.section .text
.global _start
.type _start, @function
_start:
    /*
    The bootloader has loaded us into 32-bit protected mode on a x86
    machine. We need to switch to long mode (64-bit) first.
    */

    /* First, check if long mode is supported */
    mov $0x80000000, %eax
    cpuid
    cmp $0x80000001, %eax
    jb .no_long_mode

    mov $0x80000001, %eax
    cpuid
    test $(1 << 29), %edx
    jz .no_long_mode

    /* Set up page tables */
    /* For simplicity, we'll use identity paging here */
    /* You may want to expand this for a real kernel */
    mov $p4_table, %eax
    mov %eax, %cr3

    /* Enable PAE */
    mov %cr4, %eax
    or $(1 << 5), %eax
    mov %eax, %cr4

    /* Set the long mode bit */
    mov $0xC0000080, %ecx
    rdmsr
    or $(1 << 8), %eax
    wrmsr

    /* Enable paging */
    mov %cr0, %eax
    or $(1 << 31), %eax
    mov %eax, %cr0

    /* Load the 64-bit GDT */
    lgdt gdt64_pointer

    /* Jump to long mode! */
    ljmp $0x08, $long_mode_start

.no_long_mode:
    /* Handle the case where long mode isn't supported */
    hlt
    jmp .no_long_mode

.section .rodata
gdt64:
    .quad 0x0000000000000000    /* Null descriptor */
    .quad 0x0020980000000000    /* Kernel code descriptor */
    .quad 0x0000920000000000    /* Kernel data descriptor */
gdt64_pointer:
    .word gdt64_pointer - gdt64 - 1
    .quad gdt64

.section .bss
.align 4096
p4_table:
    .skip 4096
p3_table:
    .skip 4096
p2_table:
    .skip 4096

.section .text
.code64
long_mode_start:
    /* Now in 64-bit mode */
    /* Set up the stack pointer */
    movq $stack_top, %rsp

    /*
    Clear the frame pointer (RBP) as per the System V ABI. This marks
    the deepest stack frame.
    */
    xorq %rbp, %rbp

    /*
    Enter the high-level kernel. The ABI requires the stack is 16-byte
    aligned at the time of the call instruction.
    */
    call kernel_main

    /*
    If the system has nothing more to do, put the computer into an
    infinite loop.
    */
    cli
1:  hlt
    jmp 1b

/*
Set the size of the _start symbol to the current location '.' minus its start.
This is useful when debugging or when you implement call tracing.
*/
.size _start, . - _start