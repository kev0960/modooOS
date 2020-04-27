#include "apic.h"

#include "../boot/kernel_paging.h"
#include "../std/printf.h"
#include "acpi.h"
#include "cpu.h"
#include "interrupt.h"
#include "kmalloc.h"
#include "paging.h"
#include "timer.h"
#include "vm.h"

namespace Kernel {
namespace {

constexpr uint32_t kAPICBaseAddrRegMSR = 0x1B;
constexpr uint32_t kAPICEnable = 0x800;
constexpr uint32_t kAPICSetBSP = 0x100;
constexpr size_t FourKB = 4 * (1 << 10);

constexpr uint32_t kEndOfInterruptOffset = 0xB0;
constexpr uint32_t kICRHighOffset = 0x310;
constexpr uint32_t kICRLowOffset = 0x300;

constexpr uint32_t kIOAPICInterruptMask = (1 << 16);

}  // namespace

void APICManager::InitLocalAPICForAPs() {
  uint32_t lo, hi;
  GetMSR(kAPICBaseAddrRegMSR, &lo, &hi);

  uint64_t apic_base_addr =
      (static_cast<uint64_t>(hi) << 32) | (lo >> 12 << 12);
  lo = (apic_base_addr | kAPICEnable);

  hi = apic_base_addr >> 32;
  SetMSR(kAPICBaseAddrRegMSR, lo, hi);
  // Enable spurious vector.
  SetRegister(0xF0, 0x1FF);
}

void APICManager::InitLocalAPIC() {
  uint32_t lo, hi;
  GetMSR(kAPICBaseAddrRegMSR, &lo, &hi);

  bool is_bsp = lo & 0x100;
  uint64_t apic_base_addr =
      (static_cast<uint64_t>(hi) << 32) | (lo >> 12 << 12);

  // Get the 4KB memory space (aligned at page boundary);
  apic_reg_addr_ = reinterpret_cast<uint64_t*>(kaligned_alloc(FourKB, FourKB));

  // Now map this address space to physical apic_base_addr.
  // This is because default apic_base_addr ix 0xFEE0'0000 which causes the
  // overflow in the kernel memory if we do the identity mapping. Thus, we have
  // to map the fetched page to this specific physical memory.
  PageTableManager::GetPageTableManager().AllocateKernelPage(
      (uint64_t)apic_reg_addr_, FourKB, apic_base_addr);

  lo = (apic_base_addr | kAPICEnable);
  if (is_bsp) {
    lo |= kAPICSetBSP;
  }

  hi = apic_base_addr >> 32;
  SetMSR(kAPICBaseAddrRegMSR, lo, hi);

  // Enable spurious vector.
  SetRegister(0xF0, 0x1FF);

  SendWakeUpAllCores();
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

void APICManager::SendWakeUpAllCores() {
  std::vector<CPUContext*> context_per_ap;
  for (uint32_t id : ACPIManager::GetACPIManager().GetCoreAPICIds()) {
    if (id == 0) {
      continue;
    }
    context_per_ap.push_back(CreateCPUSpecificInfo(id));
  }

  SetRegister(kICRHighOffset, 0);

  /*
  uint32_t kStartUp = 0b110 << 8;
  uint32_t kInit = 0b101 << 8;
  uint32_t kLevel = 1 << 14;
  uint32_t kLevelSensitive = 1 << 15;
  uint32_t kBroadcastExceptMe = 0b11 << 18;
*/

  // Broadcast INIT message to every APs.
  SetRegister(kICRLowOffset, 0xC4500);
  TimerManager::GetCurrentTimer().Sleep(100);

  // Vector is 2. That means, AP will start executing code at 0x2000.
  // We set the boot_ap.S to locate at 0x2000 through linker script.
  // Also, we are providing CPU specific information through the pointer to
  // the CPUContext* at 0x199B (right above starting code).
  //
  // Note that we have to temporarily identically map virtual address 0x2000 to
  // physical address 0x2000. This is because right after AP initiates paging,
  // it will try to execute the next command, which is not at kernel virtual
  // memory address. This will cause a PF if not mapped.
  // Note: 0x2000 ~ 0x2FFF (For codes)
  //       0x3000 ~ 0x3FFF (GDT is set up there :)
  PageTableManager::GetPageTableManager().CreateIdentityForKernel(0x2000,
                                                                  0x2000);
  for (CPUContext* context : context_per_ap) {
    uint32_t id = context->cpu_id;

    // kprintf("Wake up core %d \n", id);
    uint32_t context_phys_addr =
        KernelVirtualToPhys<CPUContext*, uint64_t>(context);
    uint32_t* right_above_starting =
        PhysToKernelVirtual<uint64_t, uint32_t*>(0x199B);
    *right_above_starting = context_phys_addr;

    SetRegister(kICRHighOffset, id << 24);
    SetRegister(kICRLowOffset, 0x04602);

    TimerManager::GetCurrentTimer().Sleep(100);
    // Loop until AP finishes the setup.
    while (!context->ap_boot_done) {
    }
  }

  kprintf("All APs are now woken up\n");
  is_multicore_enabled_ = true;

  // TODO Remove identically mapped kernel page after use.
}

// Returns Virtual address.
CPUContext* APICManager::CreateCPUSpecificInfo(uint32_t cpu_id) {
  // First create the CPU context.
  CPUContext* cpu_context =
      reinterpret_cast<CPUContext*>(kmalloc(sizeof(CPUContext)));

  cpu_context->pml4_addr = reinterpret_cast<uint64_t>(
      PageTableManager::GetPageTableManager().GetKernelPml4eBaseAddr());
  cpu_context->stack_addr =
      KernelVirtualToPhys<void*, uint64_t>(
          kaligned_alloc(KERNEL_BOOT_STACK_ALIGN, KERNEL_BOOT_STACK_SIZE)) +
      KERNEL_BOOT_STACK_SIZE;
  cpu_context->cpu_id = cpu_id;
  cpu_context->self = reinterpret_cast<uint64_t>(cpu_context);
  cpu_context->ap_boot_done = false;

  return cpu_context;
}

void APICManager::SetEndOfInterrupt() { SetRegister(kEndOfInterruptOffset, 0); }

void APICManager::InitIOAPIC() {
  static const uint64_t kIOAPICBase = 0xFEC00000;

  // Get the 4KB memory space (aligned at page boundary);
  ioapic_addr_ = reinterpret_cast<uint64_t>(kaligned_alloc(FourKB, FourKB));

  kprintf("ioapic base : %lx", kIOAPICBase);
  PageTableManager::GetPageTableManager().AllocateKernelPage(
      (uint64_t)ioapic_addr_, FourKB, kIOAPICBase);

  kprintf("IOAPIC num : %x \n", ReadIOAPICReg(0));

  uint32_t ioapic_ver = ReadIOAPICReg(1);
  kprintf("IOAPIC num redir entry : %x %x\n", ioapic_ver,
          (ioapic_ver >> 16) + 1);

  // Now disable irq ioapics.
  for (int i = 0; i < 16; i++) {
    SetIOAPICReg(0x10 + 2 * i, (i + 0x20) | kIOAPICInterruptMask);
    SetIOAPICReg(0x10 + 2 * i + 1, 0);
  }
}

uint32_t APICManager::ReadIOAPICReg(uint8_t reg_num) {
  // First select the register to use by setting IOREGSEL.
  *(uint32_t*)ioapic_addr_ = reg_num;

  // reg_num MUST be set before reading IOWIN. Hence, we put a full memory
  // barrier here.
  asm volatile("mfence" ::: "memory");

  // Now read the data from IOWIN.
  return *(uint32_t*)(ioapic_addr_ + 0x10);
}

void APICManager::SetIOAPICReg(uint8_t reg_num, uint32_t data) {
  // First select the register to use by setting IOREGSEL.
  *(uint32_t*)ioapic_addr_ = reg_num;

  // Just disable compiler memory reordering. (there is no store-store reorder
  // in x86)
  asm volatile("" ::: "memory");

  // Now set the data from IOWIN.
  *(uint32_t*)(ioapic_addr_ + 0x10) = data;
}

void APICManager::RedirectIRQs(uint8_t irq, uint8_t cpu_id) {
  SetIOAPICReg(0x10 + 2 * irq, 0x20 + irq);
  SetIOAPICReg(0x10 + 2 * irq + 1, (cpu_id << 24));

  kprintf("Redirect %d --> cpu_id %d \n", irq, cpu_id);
}

}  // namespace Kernel
