CC=~/opt/cross/bin/x86_64-elf-gcc
CFLAGS= -fno-builtin -O0 -nostdinc -nostdlib -ffreestanding -g -Wall -Wextra -Werror -I -mgeneral-regs-only -MMD
LDFLAGS = -nostdlib

VPATH=.:./libc
TARGET = hello print_simple cat input malloc_test
default: $(TARGET)
all: default

OBJ_FILES= string.o sha1.o printf.o syscall.o
OBJECTS = $(OBJ_FILES) crt0.o
HEADERS = $(wildcard *.h)

%.o: %.S
	$(CC) $(ASFLAGS) -c $< -o $@

%.o: %.c %.d
	$(CC) $(CFLAGS) -c $< -o $@

hello: $(OBJECTS) hello.o
	$(CC) $^ -Wall $(LDFLAGS) -o $@

cat: $(OBJECTS) cat.o
	$(CC) $^ -Wall $(LDFLAGS) -o $@

print_simple: $(OBJECTS) print_simple.o
	$(CC) $^ -Wall $(LDFLAGS) -o $@

malloc_test: $(OBJECTS) malloc_test.o
	$(CC) $^ -Wall $(LDFLAGS) -o $@

input: $(OBJECTS) input.o
	$(CC) $^ -Wall $(LDFLAGS) -o $@

clean:
	rm $(TARGET) $(OBJ_FILES)
