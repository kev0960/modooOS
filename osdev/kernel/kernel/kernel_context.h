#include "../std/vector.h"
#include "interrupt.h"

#define REGISTER_KERNEL_CONTEXT(ContextType, ContextName)       \
 public:                                                        \
  ContextType Get##ContextName() { return v_##ContextName; }    \
  void Set##ContextName(ContextType v) { v_##ContextName = v; } \
                                                                \
 private:                                                       \
  ContextType v_##ContextName;

namespace Kernel {

class KernelContext {
 public:
  static KernelContext& GetKernelContext() {
    static KernelContext kernel_context;
    return kernel_context;
  }

  REGISTER_KERNEL_CONTEXT(CPUInterruptHandlerArgs*, CPUInterruptHandlerArgs);
  REGISTER_KERNEL_CONTEXT(InterruptHandlerSavedRegs*,
                          InterruptHandlerSavedRegs);

 private:
  KernelContext(const KernelContext&) = delete;
  KernelContext(KernelContext&&) = delete;

  KernelContext() {}
};

}  // namespace Kernel

