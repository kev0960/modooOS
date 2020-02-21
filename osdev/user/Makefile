CC=~/opt/cross/bin/x86_64-elf-gcc
CFLAGS= -fno-builtin -O0 -nostdinc -nostdlib -ffreestanding -g -Wall -Wextra -Werror -I -mgeneral-regs-only
LDFLAGS = -nostdlib

TARGET = hello
default: $(TARGET)
all: default

OBJECTS = hello.o crt0.o
HEADERS = $(wildcard *.h)

%.o: %.S
	$(CC) $(ASFLAGS) -c $< -o $@

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LDFLAGS)
