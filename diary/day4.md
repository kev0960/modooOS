## 2 MB paging

I realized that the initial paging setup done in the boot loader does not really matter as we re-initialize the paging once we load the kernel. That means, it is much better to take an easier path; Using the 2MB paging temporarily until we fully setup the kernel paging.

## Size of the page table

For the booting process, we don't really need a lot of memory anyway. To jump into long mode, let's just allocate 1 GB of memory to the kernel for convenience. After we jump into the kernel, we will re-configure the kernel page table.

1GB = 2^30. That means we are only using

0xFFFF8000 00000000 to 0xFFFF8000 3FFFFFFF

# Enable long mode

490 of AMD 2.

Must enable long-mode control bit (EFER.LME)
Uses rdmsr, wrmsr; [usage](https://www.felixcloutier.com/x86/rdmsr)

