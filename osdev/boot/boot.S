#include "kernel_paging.h"
#include "gdt.h"
#include "multiboot2.h"

.set MULTIBOOT_HEADER_LENGTH, header_end - header_start
.set CHECKSUM, -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + MULTIBOOT_HEADER_LENGTH)

.section .multiboot
header_start:
  .long MULTIBOOT2_HEADER_MAGIC
  .long MULTIBOOT_ARCHITECTURE_I386
  .long MULTIBOOT_HEADER_LENGTH
  .long CHECKSUM

  # set multiboot tags
  .short MULTIBOOT_HEADER_TAG_END
  .short 0
  .long 8
header_end:

.code32

.section .bss
# Page-Map Level-4 Table (9 bits --> 2^9 * 8 bytes)
.comm PML4, PAGE_TABLE_SIZE, PAGE_TABLE_ALIGNMENT

# 0xFFFF8000 00000000~ 0xFFFF8000 3FFFFFFF (1GB)
# Page-Directory-Pointer table (9 bits --> 2^9 * 8 bytes)
.comm PDP_HIGH, PAGE_TABLE_SIZE, PAGE_TABLE_ALIGNMENT
.comm PDP_LOW, PAGE_TABLE_SIZE, PAGE_TABLE_ALIGNMENT

# Page-Directory table (9 bits --> 2^9 * 8 bytes)
#.comm PD, PAGE_TABLE_SIZE, PAGE_TABLE_ALIGNMENT
.comm TMP_STACK, KERNEL_BOOT_STACK_SIZE, KERNEL_BOOT_STACK_ALIGN


.data
.align 0x1000
gdt_table:
  .8byte GDT_NULL_DESCRIPTOR
  .8byte GDT_CODE_DESCRIPTOR

gdt_table_end:
    .skip (GDT_TABLE_SIZE - (gdt_table_end - gdt_table))

gdt_ptr:
     .short GDT_TABLE_SIZE - 1
     .long gdt_table

.section .text

.global _start
.type _start, @function
_start:
  cmp $0x36d76289, %eax
  jne multiboot_error
  jmp multiboot_success

multiboot_error:
  movl $0x4641494C, %eax
  movl $0xb8000, %ebx
  movb %al, (%ebx)
  hlt

multiboot_success:
  movl $TMP_STACK + KERNEL_BOOT_STACK_SIZE, %esp

  # Start initializing the PML4.
  movl $PDP_HIGH, %eax
  or $(PAGE_ENTRY_PRESENT | PAGE_ENTRY_RW), %eax
  movl %eax, PML4 + (PML4_ADDR_TO_ENTRY_INDEX(KERNEL_VIRTUAL_START) * PAGE_ENTRY_SIZE)

  movl $PDP_LOW, %eax
  or $(PAGE_ENTRY_PRESENT | PAGE_ENTRY_RW), %eax
  movl %eax, PML4 + (PML4_ADDR_TO_ENTRY_INDEX(KERNEL_PHYSICAL_START) * PAGE_ENTRY_SIZE)

  # Now intialize PDP
  movl $0, %eax
  or $(PAGE_ENTRY_PRESENT | PAGE_ENTRY_RW | PAGE_PAGE_SIZE), %eax
  movl %eax, PDP_HIGH + (PDP_ADDR_TO_ENTRY_INDEX(KERNEL_VIRTUAL_START) * PAGE_ENTRY_SIZE)
  movl %eax, PDP_LOW + (PDP_ADDR_TO_ENTRY_INDEX(KERNEL_PHYSICAL_START) * PAGE_ENTRY_SIZE)

  # When done, set CR3 register.
  movl $PML4, %eax
  movl %eax, %cr3

  movl $KERNEL_CR4, %eax
  movl %eax, %cr4

  # Long mode is enabled by setting long-mode enable control bit (EFER.LME) to 1.
  movl $MSR_EFER, %ecx
  rdmsr
  or $MSR_EFER_LME, %eax
  wrmsr

  movl %cr0, %eax
  or $CONTROL_REGISTER0_PAGE, %eax
  movl %eax, %cr0

  #movl $0x2f4b2f4f, (0xb8000)

  lgdt gdt_ptr

  # Our Code Descriptor is the second one. (+8 offset from GDT)
  ljmp $0x8, $_start64

  cli
  hlt

.code64

.global _start64
.type _start64, @function
_start64:
  // Setup segment selectors
  movw $0, %ax
  movw %ax, %ds
  movw %ax, %es
  movw %ax, %fs
  movw %ax, %gs
  movw %ax, %ss

  movq $0x1, %rax

  movb $0x5B, %al
  movl $0xb8000, %ebx
#  movb %al, (%ebx)

  call _init
  call KernelMain
  cli
  hlt

1:
  jmp 1b

