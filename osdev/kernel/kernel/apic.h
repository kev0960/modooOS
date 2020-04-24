#ifndef APIC_H
#define APIC_H

#include "../std/types.h"
#include "cpu_context.h"

namespace Kernel {

class APICManager {
 public:
  static APICManager& GetAPICManager() {
    static APICManager m;
    return m;
  }

  void InitLocalAPIC();
  void InitLocalAPICForAPs();
  uint32_t ReadRegister(size_t offset);
  void SetRegister(size_t offset, uint32_t val);

  void SendWakeUpAllCores();
  CPUContext* CreateCPUSpecificInfo(uint32_t cpu_id);

  APICManager(const APICManager&) = delete;
  APICManager& operator=(const APICManager&) = delete;

  bool IsMulticoreEnabled() const { return is_multicore_enabled_; }

  void SetEndOfInterrupt();

 private:
  APICManager() = default;

  uint64_t* apic_reg_addr_;
  volatile bool is_multicore_enabled_ = false;
};

}  // namespace Kernel

#endif
