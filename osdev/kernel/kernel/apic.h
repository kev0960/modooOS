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

  void InitIOAPIC();
  uint32_t ReadIOAPICReg(uint8_t reg_num);
  void SetIOAPICReg(uint8_t reg_num, uint32_t data);

  // Redirect #irq to cpu_id.
  void RedirectIRQs(uint8_t irq, uint8_t cpu_id);

 private:
  APICManager() = default;

  uint64_t* apic_reg_addr_;

  // Kernel mapped ioapic base address.
  uint64_t ioapic_addr_;

  volatile bool is_multicore_enabled_ = false;
};

}  // namespace Kernel

#endif
