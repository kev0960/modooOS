# Setting up the GDT

page 88 of AMD 2.

Segmentation is disabled in 64-bit mode, and code segments span all of virtual memory. In this mode, code-segment base addresses are ignored. For the purpose of virtual-address calculations, the base address is treated as if it has a value of zero.

Segment-limit checking is not performed, and both the segment-limit field and granularity (G) bit are ignored. Instead, the virtual address is checked to see if it is in canonical-address form.

The readable (R) and accessed (A) attributes in the type field are also ignored.

# Setting up the paging

page 130 of AMD 2.

## Just FYI Linux kernel memory layout

0 ~ 00007fffffffffff : User space virtual memory (~ 128 TB)
0000800000000000 ~ ffff7fffffffffff : NOT USED ("hole")
ffff800000000000 ~ ffffffffffffffff : Kernel (Note that not every part in this region is used. There are still some holes in between.)

## 4 KB paging vs 2 MB paging

~I decided to use 4 KB paging. I think 2 MB paging will introduce substantial internal fragmentation (though there is a possibility of switching to 2 MB paging later on).~

I realized that the linux kernel uses 2MB for the kernel paging (3 staging structure). That being said, I will also use 3 stage - 2 MB paging for ours too.
