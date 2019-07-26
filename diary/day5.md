# Supporting global construtor.

Related articles [calling global constructors](https://wiki.osdev.org/Calling_Global_Constructors)

I also had to cross compile gcc again to re-compile libgcc;
Note that the libgcc comes with g++ (linux native) comes with crtbegin.o and crtend.o that assumes that the address is within the user space (this is usually true because linux userspace starts with 0).

However to use it with the kernel where the address is assumed in high half, this is not compatible. This will create the link error.
