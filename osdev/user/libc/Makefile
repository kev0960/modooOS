CC=~/opt/cross/bin/x86_64-elf-gcc
LD=~/opt/cross/bin/x86_64-elf-ld

CFLAGS= -fno-builtin -O2 -nostdinc -nostdlib -ffreestanding -g -Wall -Wextra -Werror -I -mgeneral-regs-only -MMD
LDFLAGS = -nostdlib

LIBC_PATH = .

# Need to include libc object files.
LIBC_OBJS = $(patsubst %.c,%.o,$(wildcard $(LIBC_PATH)/*.c))

LIBC_DEPS = $(patsubst %.o,%.d,$(LIBC_OBJS))

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm *.d *.o

-include $(LIBC_DEPS)


