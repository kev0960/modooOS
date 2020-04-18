#include "apic.h"

#include "../std/printf.h"
#include "cpu.h"
#include "kmalloc.h"
#include "paging.h"
#include "timer.h"

namespace Kernel {
namespace {

constexpr uint32_t kAPICBaseAddrRegMSR = 0x1B;
constexpr uint32_t kAPICEnable = 0x800;
constexpr uint32_t kAPICSetBSP = 0x100;
constexpr size_t FourKB = 4 * (1 << 10);

constexpr uint32_t kICRHighOffset = 0x310;
constexpr uint32_t kICRLowOffset = 0x300;
}  // namespace

void APICManager::InitLocalAPIC() {
  uint32_t lo, hi;
  GetMSR(kAPICBaseAddrRegMSR, &lo, &hi);

  bool is_bsp = lo & 0x100;
  uint64_t apic_base_addr =
      (static_cast<uint64_t>(hi) << 32) | (lo >> 12 << 12);
  kprintf("base : %lx ", apic_base_addr);

  // Get the 4KB memory space (aligned at page boundary);
  apic_reg_addr_ = reinterpret_cast<uint64_t*>(kaligned_alloc(FourKB, FourKB));

  // Now map this address space to physical apic_base_addr.
  // This is because default apic_base_addr ix 0xFEE0'0000 which causes the
  // overflow in the kernel memory if we do the identity mapping. Thus, we have
  // to map the fetched page to this specific physical memory.
  PageTableManager::GetPageTableManager().AllocateKernelPage(
      (uint64_t)apic_reg_addr_, FourKB, apic_base_addr);

  kprintf("apic reg addr : %lx \n", apic_reg_addr_);

  lo = (apic_base_addr | kAPICEnable);
  if (is_bsp) {
    lo |= kAPICSetBSP;
  }

  hi = apic_base_addr >> 32;
  SetMSR(kAPICBaseAddrRegMSR, lo, hi);

  kprintf("versino : %x \n", ReadRegister(0x30));

  // Enable spurious vector.
  SetRegister(0x30, 0x1FF);

  SendWakeUp(1);
}

uint32_t APICManager::ReadRegister(size_t offset) {
  uint32_t* reg_addr = reinterpret_cast<uint32_t*>(
      apic_reg_addr_ + (offset / sizeof(uint64_t*)));
  return *reg_addr;
}

void APICManager::SetRegister(size_t offset, uint32_t val) {
  uint32_t* reg_addr = reinterpret_cast<uint32_t*>(
      apic_reg_addr_ + (offset / sizeof(uint64_t*)));
  *reg_addr = val;

  // Wait for write to finish by reading it.
  ReadRegister(offset);
}

void APICManager::SendWakeUp(size_t apic_id) {
  SetRegister(kICRHighOffset, apic_id << 24);

  /*
  uint32_t kStartUp = 0b110 << 8;
  uint32_t kInit = 0b101 << 8;
  uint32_t kLevel = 1 << 14;
  uint32_t kLevelSensitive = 1 << 15;
  uint32_t kBroadcastExceptMe = 0b11 << 18;
*/
  SetRegister(kICRLowOffset, 0xC4500);
  pic_timer.Sleep(100);

  kprintf("Broadcast INIT is done \n");
  SetRegister(kICRLowOffset, 0xC4602);
  kprintf("Broadcast SIPI is done \n");
}

}  // namespace Kernel
