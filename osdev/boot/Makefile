CC=~/opt/cross/bin/x86_64-elf-g++
SHARED_FLAGS = -fno-builtin -O2 -nostdinc -nostdlib -ffreestanding -g -Wall -Wextra \
               -Werror -I. -MMD -mno-red-zone -mcmodel=kernel -fno-pie -mgeneral-regs-only \
							 -fno-threadsafe-statics -fno-rtti -fno-exceptions
CXXFLAGS = $(SHARED_FLAGS)
ASFLAGS = $(SHARED_FLAGS) -Wa,--divide

OBJS := boot.o
OBJS += kmain.o string_util.o string.o interrupt.o keyboard.o timer.o printf.o kmalloc.o  scheduler.o kthread.o memory.o ata.o ext2.o
OBJS += kernel_test.o

CRTI_OBJ = crti.o
CRTBEGIN_OBJ := $(shell $(CC) $(CXXFLAGS) -print-file-name=crtbegin.o)
CRTEND_OBJ := $(shell $(CC) $(CFLAGS) -print-file-name=crtend.o)
CRTN_OBJ = crtn.o

OBJ_LINK_LIST := $(CRTI_OBJ) $(CRTBEGIN_OBJ) $(OBJS) $(CRTEND_OBJ) $(CRTN_OBJ)
DFILES = $(patsubst %.o,%.d,$(OBJS))

all: kernel

kernel: $(OBJ_LINK_LIST) kernel.ld Makefile
	$(CC) -z max-page-size=0x1000 $(CXXFLAGS) -no-pie -Wl,--build-id=none -T kernel.ld -o $@ $(OBJ_LINK_LIST)

clean:
	find -name "*~" -delete
	rm -rf $(OBJS) $(DFILES) kernel

%.o: %.cc
	$(CC) $(CXXFLAGS) -c $<

-include $(DFILES)
