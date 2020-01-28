#include "../std/types.h"

template <typename Impl>
class MockIOProviderBase {
 public:
  static Impl& GetMockIOProvider() {
    static typename Impl::Child mock_io_provider;
    return mock_io_provider;
  }

  static inline void outb(uint16_t port, uint8_t val) {
    static_cast<Impl*>(&GetMockIOProvider())->Outb(port, val);
  }

  static inline uint8_t inb(uint16_t port) {
    static_cast<Impl*>(&GetMockIOProvider())->Inb(port);
  }

  static inline void outw(uint16_t port, uint16_t val) {
    static_cast<Impl*>(&GetMockIOProvider())->Outw(port, val);
  }

  static inline uint16_t inw(uint16_t port) {
    static_cast<Impl*>(&GetMockIOProvider())->Inw(port);
  }

 protected:
  MockIOProviderBase() {}
};

template <typename OverridenProvider>
class MockIOProvider
    : public MockIOProviderBase<MockIOProvider<OverridenProvider>> {
 public:
  using Child = OverridenProvider;

  // Default behaviors are defined here.
  virtual void Outb(uint16_t port, uint8_t val) {}
  virtual void Inb(uint16_t port) {}
  virtual void Outw(uint16_t port, uint16_t val) {}
  virtual void Inw(uint16_t port) {}

  MockIOProvider() = default;
};

class MockIO : public MockIOProvider<MockIO> {
  // Implement user's own mock io.
  void Outb(uint16_t port, uint8_t val) override {
  }
  void Outw(uint16_t port, uint16_t val) override {
  }
};

