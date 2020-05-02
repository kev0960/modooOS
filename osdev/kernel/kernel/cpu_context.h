#ifndef CPU_CONTEXT
#define CPU_CONTEXT

#include "../std/types.h"
#include "cpu.h"
#include "kmalloc.h"

namespace Kernel {

constexpr int kGSBaseMSR = 0xC0000101;

// This is a per-cpu specific information (e.g stack address start location)
// with some general info (e.g page table location).
struct CPUContext {
  uint32_t pml4_addr;
  uint32_t stack_addr;
  uint32_t cpu_id;
  uint32_t reserved;

  // Self contains the address of the CPUContext.
  // This is because mov %gs:0, %eax is equivalent to mov [%gs:0], %eax.
  // Thus, we have to store the address of the memory that contains the ADDRESS
  // of the CPUContext. Hence, we need to copy teh addresss of 'self' to gs.
  uint64_t self;
  volatile bool ap_boot_done;
} __attribute__((packed));

class CPUContextManager {
 public:
  void SetCPUContext(CPUContext* cpu_context) {
    uint64_t self_addr = reinterpret_cast<uint64_t>(&cpu_context->self);
    CPURegsAccessProvider::SetMSR(kGSBaseMSR, self_addr, self_addr >> 32);
  }

  static uint32_t GetCurrentCPUId() {
    return GetCPUContextManager().GetCPUContext()->cpu_id;
  }

  void SetCPUContext(uint32_t cpu_id) {
    CPUContext* cpu_context =
        reinterpret_cast<CPUContext*>(kmalloc(sizeof(CPUContext)));
    cpu_context->cpu_id = cpu_id;
    cpu_context->self = reinterpret_cast<uint64_t>(cpu_context);
    SetCPUContext(cpu_context);
  }

  CPUContext* GetCPUContext() {
    CPUContext* cpu_context;
    asm volatile("movq %%gs:0, %0" : "=r"(cpu_context)::);
    return cpu_context;
  }

  static CPUContextManager& GetCPUContextManager() {
    static CPUContextManager m;
    return m;
  }

 private:
  CPUContextManager() = default;
};

}  // namespace Kernel

#endif
