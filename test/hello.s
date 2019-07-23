  .global main 

  .text
main:
  mov $1, %rax # syscall 1 is a write
  mov $1, %rdi
  mov $message, %rsi
  mov $13, %rdx
  syscall # write(1, message, 13)

  .data
message:
  .string "Hello, World\n"
