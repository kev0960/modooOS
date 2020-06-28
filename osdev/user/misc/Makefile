CC=~/opt/cross/bin/x86_64-elf-gcc
CFLAGS= -fno-builtin -O0 -nostdinc -nostdlib -ffreestanding -g -Wall -Wextra -Werror -mgeneral-regs-only -MMD
LDFLAGS = -nostdlib
LIBC_PATH = ../libc
INCLUDE = -I$(LIBC_PATH)

OBJ_PATH = ./bin

TARGET = hello input malloc_test print_simple string_test noblock

TARGET_PATH = $(addprefix $(OBJ_PATH)/,$(TARGET))
DEPS = $(addsuffix .d,$(TARGET_PATH))

# Need to include libc object files.
LIBC_OBJS = $(patsubst %.c,%.o,$(wildcard $(LIBC_PATH)/*.c))
LIBC_OBJS += $(patsubst %.S,%.o,$(wildcard $(LIBC_PATH)/*.S))

LIBC_DEPS = $(patsubst %.o,%.d,$(LIBC_OBJS))

$(info $(LIBC_OBJS))
$(info $(LIBC_DEPS))

all: $(LIBC_OBJS) $(TARGET_PATH)

$(OBJ_PATH)/%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) $(LDFLAGS) -c $< -o $@

$(LIBC_PATH)/%.o: $(LIBC_PATH)/%.c
	$(CC) $(CFLAGS) $(INCLUDE) $(LDFLAGS) -c $< -o $@

$(OBJ_PATH)/hello: $(OBJ_PATH)/hello.o $(LIBC_OBJS)
	$(CC) $(LIBC_OBJS) $< $(CFLAGS) $(LDFLAGS) -o $@

$(OBJ_PATH)/input: $(OBJ_PATH)/input.o $(LIBC_OBJS)
	$(CC) $(LIBC_OBJS) $< $(CFLAGS) $(LDFLAGS) -o $@

$(OBJ_PATH)/malloc_test: $(OBJ_PATH)/malloc_test.o $(LIBC_OBJS)
	$(CC) $(LIBC_OBJS) $< $(CFLAGS) $(LDFLAGS) -o $@

$(OBJ_PATH)/print_simple: $(OBJ_PATH)/print_simple.o $(LIBC_OBJS)
	$(CC) $(LIBC_OBJS) $< $(CFLAGS) $(LDFLAGS) -o $@

$(OBJ_PATH)/string_test: $(OBJ_PATH)/string_test.o $(LIBC_OBJS)
	$(CC) $(LIBC_OBJS) $< $(CFLAGS) $(LDFLAGS) -o $@

$(OBJ_PATH)/noblock: $(OBJ_PATH)/noblock.o $(LIBC_OBJS)
	$(CC) $(LIBC_OBJS) $< $(CFLAGS) $(LDFLAGS) -o $@

.PHONY: clean
clean:
	rm ./bin/*
	rm $(LIBC_PATH)/*.o

-include $(DEPS)
-include $(LIBC_DEPS)

