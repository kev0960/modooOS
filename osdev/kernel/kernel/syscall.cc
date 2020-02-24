#include "syscall.h"
#include "../std/printf.h"
#include "cpp_macro.h"
#include "cpu.h"
#include "kthread.h"

#define SET_KERNEL_THREAD_TOP_OFFSET(offset, reg) \
  SET_KERNEL_THREAD_TOP_OFFSET_(offset, reg)
#define SET_KERNEL_THREAD_TOP_OFFSET_(offset, reg) \
  "movq " #offset "(%%" #reg "), %%rsp \n"

namespace Kernel {

static constexpr uint32_t kLSTAR_MSR = 0xC000'0082;
static constexpr uint32_t kSTAR_MSR = 0xC000'0081;
static constexpr uint32_t kFMASK_MSR = 0xC000'0084;
static constexpr uint32_t kEEFR_MSR = 0xC000'0080;
static constexpr uint32_t kEnableSyscallExtensionBit = 0x1;

// WARNING Caller of this function MUST save RBX and R12.

// This function is called upon "syscall" instruction from the user process.
//  When the syscall is invoked,
//  * RCX holds the address of the instruction following syscall.
//    This is the place where it should go back when the syscall is done.
//  * R11 holds the previous RFLAGS. Current RFLAGS is masked by FMASK_MSR
//  * syscall loads CS and SS from STAR_MSR.
//  * it does NOT change RSP. We need to save user process's RSP and
//  * rax : syscall number.
//  * rdi : first arg
//  * rsi : second arg
//  * rdx : third arg
//  * r10 : fourth
//  * r8  : fifth
//  * r9  : sixth.
__attribute__((naked)) void SyscallHandlerAsm() {
  asm volatile(
      "movq %%rsp, %%r12\n"
      "movq %%fs:0, %%rbx\n" // RBX = CurrentThread()
      SET_KERNEL_THREAD_TOP_OFFSET(KERNEL_THREAD_SAVED_KERNEL_TOP_OFFSET, rbx)
      "push %%r12\n" // Save Previous RSP.
      "push %%r11\n" // Save Previous RFLAGS.
      "push %%rcx\n" // Save Previous RIP.

      "push %%r9\n"
      "movq %%r8, %%r9\n" // Fifth syscall arg to arg5
      "movq %%r10, %%r8\n" // Fourth syscall arg to arg4
      "movq %%rdx, %%rcx\n" // Third syscall arg to arg3
      "movq %%rsi, %%rdx\n" // Second syscall arg to arg2
      "movq %%rdi, %%rsi\n" // First syscall arg to arg1.
      "movq %%rax, %%rdi\n" // EAX to syscall_num
      "call SyscallHandlerCaller\n"
      "pop %%r9\n"
      "pop %%rcx\n"
      "pop %%r11\n"
      "pop %%r12\n"
      "movq %%r12, %%rsp\n"
      "sysretq\n":::
      );
}

SyscallManager::SyscallManager() {
  // Enable Syscall/Sysret
  uint32_t eefr_lo, eefr_hi;
  CPURegsAccessProvider::GetMSR(kEEFR_MSR, &eefr_lo, &eefr_hi);
  eefr_lo = eefr_lo | kEnableSyscallExtensionBit;  // Enable syscall extension.
  CPURegsAccessProvider::SetMSR(kEEFR_MSR, eefr_lo, eefr_hi);

  uint64_t syscall_handler_addr = reinterpret_cast<uint64_t>(SyscallHandlerAsm);

  // Register syscall handler asm.
  CPURegsAccessProvider::SetMSR(kLSTAR_MSR, syscall_handler_addr,
                                syscall_handler_addr >> 32);

  // Register MASK for RFLAGS.
  uint64_t rflags_mask = 0x200;
  CPURegsAccessProvider::SetMSR(kFMASK_MSR, rflags_mask, rflags_mask >> 32);

  // Register User CS/SS upon sysret.
  uint32_t star_lo, star_hi;
  CPURegsAccessProvider::GetMSR(kSTAR_MSR, &star_lo, &star_hi);

  // CS for user (1B) and CS for kernel (08). SS is +8 from each CS.
  star_hi = 0x00130008;

  CPURegsAccessProvider::SetMSR(kSTAR_MSR, star_lo, star_hi);
}

void SyscallManager::SyscallHandler(uint64_t syscall_num) {
  /*
  kprintf("Syscall %d %lx %lx\n", syscall_num,
          KernelThread::CurrentThread()->Id(),
          CPURegsAccessProvider::GetRFlags());*/
  UNUSED(syscall_num);
}

extern "C" void SyscallHandlerCaller(uint64_t syscall_num, uint64_t arg1,
                                     uint64_t arg2, uint64_t arg3,
                                     uint64_t arg4, uint64_t arg5,
                                     uint64_t arg6) {
  /*kprintf("Syscall : %d %d %d %d %d %d %d\n", syscall_num, arg1, arg2, arg3,
          arg4, arg5, arg6);*/

  uint64_t* return_addr =
      (uint64_t*)KernelThread::CurrentThread()->GetKernelStackTop() -
      4;  // (uint64_t*)__builtin_frame_address(0);
  /*
  kprintf("r9 : %lx rip : %lx rflags: %lx rsp: %lx ret: %lx\n", return_addr[0],
          return_addr[1], return_addr[2], return_addr[3], return_addr);
         */
  // kprintf("Return addr : %lx \n", __builtin_return_address(0));
  SyscallManager::GetSyscallManager().SyscallHandler(syscall_num);
  UNUSED(arg1);
  UNUSED(arg2);
  UNUSED(arg3);
  UNUSED(arg4);
  UNUSED(arg5);
  UNUSED(arg6);
  UNUSED(return_addr);
  kprintf("sycall done %lx %lx %lx\n", return_addr[3], return_addr,
          CPURegsAccessProvider::GetRFlags());
}

}  // namespace Kernel
