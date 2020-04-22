// Custom assembly based interrupt handler for timer and context switch int.

.code64

.macro push_regs
  push %r15
  push %r14
  push %r13
  push %r12
  push %r11
  push %r10
  push %r9
  push %r8
  push %rdi
  push %rsi
  push %rdx
  push %rcx
  push %rbx
  push %rax
  push %rbp
.endm

.macro pop_regs
  pop %rbp
  pop %rax
  pop %rbx
  pop %rcx
  pop %rdx
  pop %rsi
  pop %rdi
  pop %r8
  pop %r9
  pop %r10
  pop %r11
  pop %r12
  pop %r13
  pop %r14
  pop %r15
.endm

.global TimerInterruptHandler
.type TimerInterruptHandler, @function
TimerInterruptHandler:
  push %r15
  push %r14
  push %r13
  push %r12
  push %r11
  push %r10
  push %r9
  push %r8
  push %rdi
  push %rsi
  push %rdx
  push %rcx
  push %rbx
  push %rax
  push %rbp

  // Set InterruptHandlerSavedRegs to rax.
  movq %rsp, %rsi
  lea 0x78(%rsp), %rdi
  callq TimerCaller

  mov $0x20, %eax
  out %al, $0x20

  pop %rbp
  pop %rax
  pop %rbx
  pop %rcx
  pop %rdx
  pop %rsi
  pop %rdi
  pop %r8
  pop %r9
  pop %r10
  pop %r11
  pop %r12
  pop %r13
  pop %r14
  pop %r15
  iretq

.global CustomContextSwitchInterruptHandler
.type CustomContextSwitchInterruptHandler, @function
CustomContextSwitchInterruptHandler:
  push %r15
  push %r14
  push %r13
  push %r12
  push %r11
  push %r10
  push %r9
  push %r8
  push %rdi
  push %rsi
  push %rdx
  push %rcx
  push %rbx
  push %rax
  push %rbp

  // Set InterruptHandlerSavedRegs to rax.
  movq %rsp, %rsi
  lea 0x78(%rsp), %rdi
  callq YieldInInterruptHandlerCaller

  mov $0x20, %eax
  out %al, $0x20

  pop %rbp
  pop %rax
  pop %rbx
  pop %rcx
  pop %rdx
  pop %rsi
  pop %rdi
  pop %r8
  pop %r9
  pop %r10
  pop %r11
  pop %r12
  pop %r13
  pop %r14
  pop %r15

  iretq

.global PageFaultInterruptHandler
.type PageFaultInterruptHandler, @function
PageFaultInterruptHandler:
  push %r15
  push %r14
  push %r13
  push %r12
  push %r11
  push %r10
  push %r9
  push %r8
  push %rdi
  push %rsi
  push %rdx
  push %rcx
  push %rbx
  push %rax
  push %rbp

  // Set InterruptHandlerSavedRegs to rax.
  movq %rsp, %rsi
  lea 0x80(%rsp), %rdi
  callq PageFaultInterruptHandlerCaller

  mov $0x20, %eax
  out %al, $0x20

  pop %rbp
  pop %rax
  pop %rbx
  pop %rcx
  pop %rdx
  pop %rsi
  pop %rdi
  pop %r8
  pop %r9
  pop %r10
  pop %r11
  pop %r12
  pop %r13
  pop %r14
  pop %r15
  addq $8, %rsp // Pop the error code.

  iretq
