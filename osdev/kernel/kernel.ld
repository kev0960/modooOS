ENTRY(_start)

SECTIONS
{
  . = 0x2000;
  .boot_ap_text : {
    ./obj/boot_ap.o (.text)
  }
  .boot_ap_ro_data : {
    ./obj/boot_ap.o (.ro_data)
  }
  .boot_ap_data : {
    ./obj/boot_ap.o (.data)
  }
  .boot_ap_bass : {
    ./obj/boot_ap.o (.bss)
    ./obj/boot_ap.o (COMMON)
  }

  . = 4M;
  _kernel_physical_start = .;

  .boottext :
    {
      ./obj/boot.o (.multiboot)
      ./obj/boot.o (.text)
    }
  .bootrodata :
    {
      ./obj/boot.o (.rodata)
    }
  .bootdata :
    {
      ./obj/boot.o (.data)
    }
  .bootbss :
    {
      ./obj/boot.o (.bss)
      ./obj/boot.o (COMMON)
    }

  . = ALIGN(0x1000);
  _boot_end = .;

  . += 0xFFFFFFFF80000000;
  _kernel_virtual_start = .;
  .text : AT(_boot_end)
  {
    *(.text)
    *(.text.*)
    *(.fini)
    *(.init)
  }

  . = ALIGN(0x1000);

  .rodata : AT ( (LOADADDR (.text) + SIZEOF (.text) + 0xFFF) & 0xFFFFFFFFFFFFF000 )
  {
    *(.rodata)
    *(.rodata.*) /* For linking crtbegin.o */
  }

  . = ALIGN(0x1000);

  .data : AT ( (LOADADDR (.rodata) + SIZEOF (.rodata) + 0xFFF) & 0xFFFFFFFFFFFFF000 )
  {
    *(.data)
  }

  . = ALIGN(0x1000);

  .bss : AT ( (LOADADDR (.data) + SIZEOF (.data) + 0xFFF) & 0xFFFFFFFFFFFFF000 )
  {
    *(COMMON)
    *(.bss)
    *(.eh_frame)
  }

  _kernel_virtual_end = .;

  _kernel_physical_end = (LOADADDR (.bss) + SIZEOF (.bss) + 0xFFF) & 0xFFFFFFFFFFFFF000;
}
