CC=~/opt/cross/bin/x86_64-elf-g++
SHARED_FLAGS = -fno-builtin -O2 -nostdinc -nostdlib -ffreestanding -g -Wall -Wextra \
               -Werror -I ./std -MMD -mno-red-zone -mcmodel=kernel -fno-pie -mgeneral-regs-only \
							 -fno-threadsafe-statics -fno-rtti -fno-exceptions -std=c++17 -fno-builtin
CXXFLAGS = $(SHARED_FLAGS)
ASFLAGS = $(SHARED_FLAGS) -Wa,--divide

VPATH = ./kernel:./boot:./std:./test:./kernel/fs:./kernel/sys
OBJDIR = ./obj

BOOT_DIR = ./boot
KERNEL_DIR = ./kernel
STD_DIR = ./std
FS_DIR = $(KERNEL_DIR)/fs
SYS_DIR = $(KERNEL_DIR)/sys

SRCS = $(notdir $(wildcard $(BOOT_DIR)/*.S))
SRCS += $(notdir $(wildcard $(KERNEL_DIR)/*.cc $(KERNEL_DIR)/*.S))
SRCS += $(notdir $(wildcard $(FS_DIR)/*.cc $(FS_DIR)/*.S))
SRCS += $(notdir $(wildcard $(SYS_DIR)/*.cc $(SYS_DIR)/*.S))
SRCS += $(notdir $(wildcard $(STD_DIR)/*.cc $(STD_DIR)/*.S))

# Add test here.
SRCS += kernel_test.cc
$(info $(SRCS))

OBJS := $(addsuffix .o, $(basename $(SRCS)))

CRTI_OBJ = ./obj/crti.o
CRTBEGIN_OBJ := $(shell $(CC) $(CXXFLAGS) -print-file-name=crtbegin.o)
CRTEND_OBJ := $(shell $(CC) $(CFLAGS) -print-file-name=crtend.o)
CRTN_OBJ = ./obj/crtn.o

OBJS_IN_OBJ_DIR = $(addprefix $(OBJDIR)/, $(OBJS))
OBJ_LIST = $(CRTI_OBJ) $(CRTBEGIN_OBJ) $(OBJS_IN_OBJ_DIR) $(CRTEND_OBJ) $(CRTN_OBJ)
DFILES = $(addprefix $(OBJDIR)/, $(patsubst %.o,%.d,$(OBJS)))

all: kernel

kernel: $(OBJ_LIST) kernel.ld Makefile
	$(CC) -z max-page-size=0x1000 $(CXXFLAGS) -no-pie -Wl,--build-id=none -T kernel.ld -o kernel.bin $(OBJ_LIST) -lgcc

$(info $(OBJ_LIST))
$(info $(OBJS))

clean:
	find -name "*~" -delete
	rm -rf $(OBJS_IN_OBJ_DIR) $(DFILES)

$(OBJDIR)/%.o: %.cc
	$(CC) $(CXXFLAGS) -c $< -o $@

obj/boot_ap.o: boot_ap.S
	$(CC) $(ASFLAGS) -c $< -o $@

obj/boot.o: boot.S
	$(CC) $(ASFLAGS) -c $< -o $@

obj/interrupt_handler.o: interrupt_handler.S
	$(CC) $(ASFLAGS) -c $< -o $@

-include $(DFILES)
