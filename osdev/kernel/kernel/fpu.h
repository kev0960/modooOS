#include "../std/types.h"

namespace Kernel {

class FPUManager {
 public:
  static FPUManager& GetFPUManager() {
    static FPUManager fpu_manager;
    return fpu_manager;
  }

  void InitFPU();

 private:
  FPUManager() = default;

  bool HasSSE();
};

}  // namespace Kernel
