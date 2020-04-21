#include "gdt.h"
#include "kernel_paging.h"

.code16

.data
.align 0x1000
gdt_table:
  .8byte GDT_NULL_DESCRIPTOR
  .8byte GDT_CODE_DESCRIPTOR  # This one is 64bit :p
  .8byte GDT_DATA_DESCRIPTOR16
  .8byte GDT_CODE_DESCRIPTOR16
  .8byte GDT_DATA_DESCRIPTOR32
  .8byte GDT_CODE_DESCRIPTOR32

gdt_table_end:
    .skip (GDT_TABLE_SIZE - (gdt_table_end - gdt_table))

gdt_ptr:
     .short GDT_TABLE_SIZE - 1
     .long gdt_table

.text
.global _start_ap
_start_ap:
  cli # Disable Interrupt.

  xor %ax, %ax
  mov %ax, %ds # Set segments as NULL segments.
  mov %ax, %es
  mov %ax, %ss

  lgdt gdt_ptr
  mov %cr0, %eax
  or $1, %al
  mov %eax, %cr0

  ljmp $0x28, $_start32_ap

.code32
_start32_ap:
  mov $0x20, %ax
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %ss

  # At this point, we are not really using FS and GS. Let's just give them
  # null descriptor.
  xor %ax, %ax
  mov %ax, %fs
  mov %ax, %gs

  # Now we have to enable paging. We are going to use provided CPU context
  # at 0x199B.
  mov $0x199B, %ebx
  mov (%ebx), %ebx  # ebx now holds address of CPUContext
  mov (%ebx), %eax  # eax = CPUContext->pml4_addr
  mov %eax, %cr3    # Set PML4E address.
  mov 4(%ebx), %eax # eax = CPUContext->stack_addr
  mov %eax, %esp    # Set stack addr.

  mov 16(%ebx), %edi # edi = CPUContext->self (low 4 bytes)
  mov 20(%ebx), %esi # esi = CPUContext->self (high 4 bytes)

  mov $KERNEL_CR4, %eax
  mov %eax, %cr4    # Enable Paging :)

  mov $MSR_EFER, %ecx
  rdmsr
  or $MSR_EFER_LME, %eax
  wrmsr

  mov %cr0, %eax
  or $CONTROL_REGISTER0_PAGE, %eax
  mov %eax, %cr0

  ljmp $0x8, $_start64_ap

.code64

.global _start64_ap
.type _start64_ap, @function
_start64_ap:
   // Setup segment selectors
  movw $0, %ax
  movw %ax, %ds
  movw %ax, %es
  movw %ax, %fs
  movw %ax, %gs
  movw %ax, %ss

  // Add kernel virtual offset.
  // Note that rdi and rsi contains the address to CPUContext
  addq $0xffffffff80000000, %rsp
  call KernelMainForAP
spin:
  jmp spin


