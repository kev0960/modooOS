CC=~/opt/cross/bin/x86_64-elf-gcc
CFLAGS= -fno-builtin -O0 -nostdinc -nostdlib -ffreestanding -g -Wall -Wextra -Werror -I -mgeneral-regs-only -MMD
LDFLAGS = -nostdlib
LIBC_PATH = ../libc
INCLUDE = -I$(LIBC_PATH)

OBJ_PATH = ./bin

TARGET = cat ls pwd

TARGET_PATH = $(addprefix $(OBJ_PATH)/,$(TARGET))
DEPS = $(addsuffix .d,$(TARGET_PATH))

# Need to include libc object files.
LIBC_OBJS = $(patsubst %.c,%.o,$(wildcard $(LIBC_PATH)/*.c))
LIBC_OBJS += $(patsubst %.S,%.o,$(wildcard $(LIBC_PATH)/*.S))

LIBC_DEPS = $(patsubst %.o,%.d,$(LIBC_OBJS))

$(info $(LIBC_OBJS))

all: $(LIBC_OBJS) $(TARGET_PATH)

$(OBJ_PATH)/%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) $(LDFLAGS) -c $< -o $@

$(LIBC_PATH)/%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) $(LDFLAGS) -c $< -o $@

$(OBJ_PATH)/cat: $(OBJ_PATH)/cat.o
	$(CC) $(LIBC_OBJS) $< $(CFLAGS) $(LDFLAGS) -o $@

$(OBJ_PATH)/ls: $(OBJ_PATH)/ls.o
	$(CC) $(LIBC_OBJS) $< $(CFLAGS) $(LDFLAGS) -o $@

$(OBJ_PATH)/pwd: $(OBJ_PATH)/pwd.o
	$(CC) $(LIBC_OBJS) $< $(CFLAGS) $(LDFLAGS) -o $@

.PHONY: clean
clean:
	rm ./bin/*
	rm ../libc/*.o

-include $(DEPS)
-include $(LIBC_DEPS)

