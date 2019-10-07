CC=~/opt/cross/bin/x86_64-elf-g++
SHARED_FLAGS = -nostdlib -fno-rtti -fno-threadsafe-statics

CXXFLAGS = $(SHARED_FLAGS)
ASFLAGS = $(SHARED_FLAGS) -Wa,--divide

COMMON_OBJS := syscall_asm.o
COMMON_OBJS += write.o printf.o

CRTI_OBJ = crti.o
CRTBEGIN_OBJ := $(shell $(CC) -print-file-name=crtbegin.o)
CRTEND_OBJ := $(shell $(CC) -print-file-name=crtend.o)
CRTN_OBJ = crtn.o

OBJ_LINK_LIST := $(CRTI_OBJ) $(CRTBEGIN_OBJ) $(COMMON_OBJS) $(CRTEND_OBJ) $(CRTN_OBJ)

all: string_util_test

string_util_test: $(OBJ_LINK_LIST) string_util_test.o Makefile
	$(CC) $(CXXFLAGS) -T test.ld -o $@ $(CRTI_OBJ) $(CRTBEGIN_OBJ) ../boot/string_util.o string_util_test.o $(COMMON_OBJS) $(CRTEND_OBJ) $(CRTN_OBJ)

clean:
	find -name "*~" -delete
	rm -rf $(COMMON_OBJS) string_util_test.o

%.o: %.cc
	$(CC) $(CXXFLAGS) -c $<

-include $(DFLAGS)
