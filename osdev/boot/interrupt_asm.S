.code64

// Interrupt handler for CPU exceptions and software interrupts.

// When the error code is not supplied, we just push dummy error code just for
// convenience.
.macro create_cpu_ih_no_err_code int_num
.global _cpu_ih\int_num
.type _cpu_ih\int_num, @function
_cpu_ih\int_num:
  cli // Clear interrupt.

  push $0 // Push dummy error code.

  // Push interrupt index.
  push $\int_num
  push %rbp

  jmp cpu_ih_common_handler
.endm

.macro create_cpu_ih int_num
.global _cpu_ih\int_num
.type _cpu_ih\int_num, @function
_cpu_ih\int_num:
  cli // Clear interrupt.

  // Push interrupt index.
  push $\int_num
  push %rbp

  jmp cpu_ih_common_handler
.endm

create_cpu_ih_no_err_code 0 // #DE
create_cpu_ih_no_err_code 1 // #DB
create_cpu_ih_no_err_code 2 // NMI Interrupt
create_cpu_ih_no_err_code 3 // #BP
create_cpu_ih_no_err_code 4 // #OF
create_cpu_ih_no_err_code 5 // #BR
create_cpu_ih_no_err_code 6 // #UD
create_cpu_ih_no_err_code 7 // #NM
create_cpu_ih 8 // #DF
create_cpu_ih_no_err_code 9 // #NM
create_cpu_ih 10 // #TS
create_cpu_ih 11 // #NP
create_cpu_ih 12 // #SS
create_cpu_ih 13 // #GP
create_cpu_ih 14 // #PF
create_cpu_ih_no_err_code 15 // Reserved; DO NOT USE
create_cpu_ih_no_err_code 16 // #MF
create_cpu_ih 17 // #AC
create_cpu_ih_no_err_code 18 // #MC
create_cpu_ih_no_err_code 19 // #XM
create_cpu_ih_no_err_code 20 // #VE
create_cpu_ih_no_err_code 21 // Reserved; DO NOT USE
create_cpu_ih_no_err_code 22 // Reserved; DO NOT USE
create_cpu_ih_no_err_code 23 // Reserved; DO NOT USE
create_cpu_ih_no_err_code 24 // Reserved; DO NOT USE
create_cpu_ih_no_err_code 25 // Reserved; DO NOT USE
create_cpu_ih_no_err_code 26 // Reserved; DO NOT USE
create_cpu_ih_no_err_code 27 // Reserved; DO NOT USE
create_cpu_ih_no_err_code 28 // Reserved; DO NOT USE
create_cpu_ih_no_err_code 29 // Reserved; DO NOT USE
create_cpu_ih_no_err_code 30 // Reserved; DO NOT USE
create_cpu_ih_no_err_code 31 // Reserved; DO NOT USE

cpu_ih_common_handler:
   call CPUInterruptHandler

   // Pop interrupt index and rbp
   addq $16, %rsp
   iretq
