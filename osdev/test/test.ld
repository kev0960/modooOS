ENTRY(_start)

SECTIONS
{
  . = 0x400000;

  .text : {
    *(.text)
    *(.init)
    *(.fini)
  }

  .rodata : {
    *(.rodata)
  }

  .data : {
    *(.data)
  }

  .bss : {
    *(COMMON)
    *(.bss)
  }
}
