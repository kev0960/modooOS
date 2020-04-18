#include "gdt.h"

.code16

.data
.align 0x1000
gdt_table:
  .8byte GDT_NULL_DESCRIPTOR
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

  ljmp $0x20, $_start32_ap

print_in_real:
  mov $0, %bh
  mov $0, %bl
  mov $0xE, %ah
  int $0x10
  ret

.code32
_start32_ap:
  xor %ax, %ax

  mov %ax, %es
  mov %ax, %fs
  mov %ax, %gs

spin:
  jmp spin

