#include "../std/types.h"
#include "mock.h"

namespace Kernel {
namespace kernel_test {

template <typename Impl>
class MockCPUAccessProviderBase {
 public:
  static inline uint64_t GetRFlags() {
    return static_cast<Impl *>(&GetMockCPUAccessProvider())->MockGetRFlags();
  }

  static inline void SetRFlags(uint64_t rflags) {
    static_cast<Impl *>(&GetMockCPUAccessProvider())->MockSetRFlags(rflags);
  }

  static inline bool IsInterruptEnabled() {
    return static_cast<Impl *>(&GetMockCPUAccessProvider())
        ->MockIsInterruptEnabled();
  }

  // Get and Set model specific registers.
  // Courtesy of osdev. https://wiki.osdev.org/Model_Specific_Registers
  static inline void GetMSR(uint32_t msr, uint32_t *lo, uint32_t *hi) {
    static_cast<Impl *>(&GetMockCPUAccessProvider())->MockGetMSR(msr, lo, hi);
  }

  static inline void SetMSR(uint32_t msr, uint32_t lo, uint32_t hi) {
    static_cast<Impl *>(&GetMockCPUAccessProvider())->MockSetMSR(msr, lo, hi);
  }

  static inline void EnableInterrupt() {
    static_cast<Impl *>(&GetMockCPUAccessProvider())->MockEnableInterrupt();
  }
  static inline void DisableInterrupt() {
    static_cast<Impl *>(&GetMockCPUAccessProvider())->MockDisableInterrupt();
  }

  static inline uint64_t ReadCR0() {
    return static_cast<Impl *>(&GetMockCPUAccessProvider())->MockReadCR0();
  }

  static inline void SetCR3(uint64_t cr3) {
    static_cast<Impl *>(&GetMockCPUAccessProvider())->MockSetCR3(cr3);
  }

  static Impl &GetMockCPUAccessProvider() {
    static typename Impl::Child mock_cpu_access_provider;
    return mock_cpu_access_provider;
  }

 protected:
  MockCPUAccessProviderBase() {}
};

template <typename OverridenProvider>
class MockCPUAccessProviderDefault
    : public MockCPUAccessProviderBase<
          MockCPUAccessProviderDefault<OverridenProvider>> {
 public:
  using Child = OverridenProvider;

  static Child &GetMock() {
    return *static_cast<Child *>(
        &MockCPUAccessProviderBase<MockCPUAccessProviderDefault<
            OverridenProvider>>::GetMockCPUAccessProvider());
  }

  virtual uint64_t MockGetRFlags() {
    DEFAULT_MOCK0;
    return 0;
  }

  virtual void MockSetRFlags(uint64_t rflags) { DEFAULT_MOCK1(rflags); }

  virtual bool MockIsInterruptEnabled() {
    DEFAULT_MOCK0;
    return false;
  }

  // Get and Set model specific registers.
  // Courtesy of osdev. https://wiki.osdev.org/Model_Specific_Registers
  virtual void MockGetMSR(uint32_t msr, uint32_t *lo, uint32_t *hi) {
    DEFAULT_MOCK3(msr, lo, hi);
  }

  virtual void MockSetMSR(uint32_t msr, uint32_t lo, uint32_t hi) {
    DEFAULT_MOCK3(msr, lo, hi);
  }

  virtual void MockEnableInterrupt() { DEFAULT_MOCK0; }
  virtual void MockDisableInterrupt() { DEFAULT_MOCK0; }

  virtual uint64_t MockReadCR0() {
    DEFAULT_MOCK0;
    return 0;
  }

  virtual void MockSetCR3(uint64_t cr3) { DEFAULT_MOCK1(cr3); }

  MockCPUAccessProviderDefault() = default;
};

// To create a mock CPUAccess provider, use the following pattern.
// class MockCPUAccessProvider : public
// MockCPUAccessProviderDefault<MockCPUAccessProvider> {
//    // Implement functions that you want to override.
//    void Outb(uint16_t porst, uint8_t val) override {}
// };

}  // namespace kernel_test
}  // namespace Kernel
