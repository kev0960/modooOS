CC=~/opt/cross/bin/x86_64-elf-g++
SHARED_FLAGS = -fno-builtin -O2 -nostdinc -nostdlib -ffreestanding -g -Wall -Wextra \
               -Werror -I ./std -MMD -mno-red-zone -mcmodel=kernel -fno-pie -mgeneral-regs-only \
							 -fno-threadsafe-statics -fno-rtti -fno-exceptions
CXXFLAGS = $(SHARED_FLAGS)
ASFLAGS = $(SHARED_FLAGS) -Wa,--divide

VPATH = ./kernel:./boot:./std:./test
OBJDIR = ./obj

OBJS := boot.o interrupt_handler.o
OBJS += kmain.o string_util.o string.o interrupt.o keyboard.o timer.o printf.o kmalloc.o  scheduler.o kthread.o memory.o ata.o ext2.o kernel_util.o
OBJS += kernel_test.o kmalloc_test.o

CRTI_OBJ = ./obj/crti.o
CRTBEGIN_OBJ := $(shell $(CC) $(CXXFLAGS) -print-file-name=crtbegin.o)
CRTEND_OBJ := $(shell $(CC) $(CFLAGS) -print-file-name=crtend.o)
CRTN_OBJ = ./obj/crtn.o

OBJS_IN_OBJ_DIR = $(addprefix $(OBJDIR)/, $(OBJS))
OBJ_LIST = $(CRTI_OBJ) $(CRTBEGIN_OBJ) $(OBJS_IN_OBJ_DIR) $(CRTEND_OBJ) $(CRTN_OBJ)
DFILES = $(addprefix $(OBJDIR)/, $(patsubst %.o,%.d,$(OBJS)))

$(info $(DFILES))

all: kernel

kernel: $(OBJS) kernel.ld Makefile
	$(CC) -z max-page-size=0x1000 $(CXXFLAGS) -no-pie -Wl,--build-id=none -T kernel.ld -o kernel.bin $(OBJ_LIST)

clean:
	find -name "*~" -delete
	rm -rf $(OBJS) $(DFILES)

%.o: %.cc
	$(CC) $(CXXFLAGS) -c $< -o $(OBJDIR)/$@

%.o: %.S
	$(CC) $(ASFLAGS) -c $< -o $(OBJDIR)/$@

-include $(DFILES)
