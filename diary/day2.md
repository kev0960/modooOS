# Multiboot

Multiboot specification can be found [here](https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html). The important parts are;

## Layout of Multiboot2 header

Offset	Type	Field Name	Note 
0	u32	magic	required 
4	u32	architecture	required 
8	u32	header\_length	required 
12	u32	checksum	required 
16-XX		tags	required 

* magic : the magic number identifying the header, which must be the hexadecimal value 0xE85250D6. Note that we don't have to think of endian here. (The multiboot will guess the endian from this provided value.) 

* architecture : specifies the architecture. (0 - 32bit protected, 4 - 32bit MIPS).
we will jump to 32bit protected; that means we have to use 0 as architecture field.

* header\_length : Length of the multiboot header in bytes (including magic fields)
* checksum : 32 bit unsigned, when added with magic, architecture and header\_length, the total should be 0.
* tags : Information request tag is not really used. The spec says;

             +-------------------+
     u16     | type              |
     u16     | flags             |
     u32     | size              |
             +-------------------+

Tags are terminated by a tag of type ‘0’ and size ‘8’. That means, we have to provide 0 and 8.

## Compiling with multiboot2.h

You need to change #define macro to __ASSEMBLER__

# Setting up grub-mkrescue

If grub-mkrescue does not generate an iso;

-> sudo apt install grub-pc-bin

If you still see some error message (complaining about xorriso version)

-> sudo apt install xorriso
