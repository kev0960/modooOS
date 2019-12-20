# Enabling cross compiling with x86_64-elf.

1.  Download the recent versions of binutils and gcc from their mirrors. 2.
    `export PREFIX="$HOME/opt/cross" export TARGET=x86_64-elf export
    PATH="$PREFIX/bin:$PATH"`

2.  Now install binutils by running following commands.

```
../binutils-x.y.z/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
```

1.  Now install gcc. To do so, first check whether our cross compiled version of
    binutils will be used.

```
which -- $TARGET-as || echo $TARGET-as is not in the PATH
```

should show `$HOME/opt/cross/bin/x86_64-elf-as`

**YOU MUST CREATE A NEW DIRECTORY TO BUILD AND CONFIGURE GCC!**

[GOOD LINK](https://wiki.osdev.org/Building_libgcc_for_mcmodel%3Dkernel) **USE
-j (number of CPU)** option to enhance the speed of the build :)

Suppose you created a build-gcc dir.

```
cd build-gcc
../gcc-9.1.0/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers

# If configure fails, install ./contrib/prereq...
# We need to create libgcc in mcmodel=kernel and no-red-zone
make all-target-libgcc CFLAGS_FOR_TARGET='-g -O2 -mcmodel=kernel -mno-red-zone' || true
sed -i 's/PICFLAG/DISABLED_PICFLAG/g' $TARGET/libgcc/Makefile
make all-target-libgcc CFLAGS_FOR_TARGET='-g -O2 -mcmodel=kernel -mno-red-zone'
make install-gcc
make install-target-libgcc
```

It will take pretty long time. GCC is huge! (For my case it took around 2hrs)

# Learning basics.

Read page 436 of AMD programmer manual 2.

## Notes

Probably we have to build gcc and run following `make all-target-libgcc
CFLAGS_FOR_TARGET='-g -O2 -mcmodel=kernel -mno-red-zone' || true`

which might fail. We need to dist-clean in this case and re-run above command.

```
make all-target-libgcc CFLAGS_FOR_TARGET='-g -O2 -mcmodel=kernel -mno-red-zone' || true
```

# will fail with: cc1: error: code model kernel does not support PIC mode

```
sed -i 's/PICFLAG/DISABLED_PICFLAG/g' $TARGET/libgcc/Makefile
sed -i 's/PICFLAG/DISABLED_PICFLAG/g' $TARGET/no-red-zone/libgcc/Makefile
```

## Installing grub-mkrescue

```
sudo apt install xorriso
sudo apt install mtools
```

## qemu error

Boot failed: Could not read from CDROM (code 0009)
