.code64

.global _start
.global _syscall5

.type _syscall5, @function
_syscall5:
  movq %rdi, %rax
  movq %rsi, %rdi
  movq %rdx, %rsi
  movq %rcx, %rdx
  movq %r8, %r10
  movq %r9, %r8
  syscall
  ret


.type _start, @function
_start:
  call _init
  xor %rbp, %rbp /* zero out rbp */
  pop %rdi /* rdi = argc */
  movq %rsp, %rsi /* rsi = argv */
  andq $-16, %rsp /* Align stack to 16 byte boundary */
  call TestMain

  // Exit
  mov %rax, %rdi /* Set error code as the return value from main. */
  movq $60, %rax /* SYS_EXIT */
  syscall

  ret
