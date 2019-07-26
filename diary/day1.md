# Enabling cross compiling with x86_64-elf.

1. Download the recent versions of binutils and gcc from their mirrors.
2.
```
export PREFIX="$HOME/opt/cross"
export TARGET=x86_64-elf
export PATH="$PREFIX/bin:$PATH"
```

3. Now install binutils by running following commands.

```
../binutils-x.y.z/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
```

4. Now install gcc. To do so, first check whether our cross compiled version of
binutils will be used.

```
which -- $TARGET-as || echo $TARGET-as is not in the PATH
```

should show `$HOME/opt/cross/bin/x86_64-elf-as`

**YOU MUST CREATE A NEW DIRECTORY TO BUILD AND CONFIGURE GCC!**

Suppose you created a build-gcc dir.

```
cd build-gcc
../gcc-9.1.0/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers

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






