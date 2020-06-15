CC=~/opt/cross/bin/x86_64-elf-gcc
CFLAGS= -fno-builtin -O0 -nostdinc -nostdlib -ffreestanding -g -Wall -Wextra -Werror -I -mgeneral-regs-only -MMD
LDFLAGS = -nostdlib -lstdc
LIBC_PATH = ../libc
LDPATH = -L$(LIBC_PATH)
INCLUDE = -I$(LIBC_PATH)

OBJ_PATH = ./bin

TARGET = test

TARGET_PATH = $(addprefix $(OBJ_PATH)/,$(TARGET))
DEPS = $(addsuffix .d,$(TARGET_PATH))

all: $(TARGET_PATH)

$(OBJ_PATH)/%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) $(LDFLAGS) -c $< -o $@

$(OBJ_PATH)/test: $(OBJ_PATH)/test.o
	$(CC) $< $(CFLAGS) $(LDFLAGS) $(LDPATH) $(LIBC_PATH)/crt0.S -o $@

.PHONY: clean

clean:
	rm ./bin/*

-include $(DEPS)

