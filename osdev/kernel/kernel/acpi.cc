#include "acpi.h"
#include "io.h"

namespace Kernel {
namespace {

template <typename T, typename U = uint8_t*>
T ReadAndAdvance(U& dat) {
  auto t = *reinterpret_cast<T*>(dat);
  dat = reinterpret_cast<U>(reinterpret_cast<uint8_t*>(dat) + sizeof(T));
  return t;
}

}  // namespace

struct ProcessLocalAPIC {
  uint8_t acpi_processor_id;
  uint8_t apic_id;
  uint32_t flags;
} __attribute__((packed));

struct IOAPIC {
  uint8_t io_apic_id;
  uint8_t reserved;
  uint32_t io_apic_addr;
  uint32_t global_sys_intr_base;
} __attribute__((packed));

struct InterruptSourceOverride {
  uint8_t bus_src;
  uint8_t irq_src;
  uint32_t global_sys_intr;
  uint16_t flags;
} __attribute__((packed));

struct NonMaskableInterrupts {
  uint8_t acpi_processor_id;
  uint16_t flags;
  uint8_t lint_num;
} __attribute__((packed));

struct LocalAPICAddressOverride {
  uint16_t reserved;
  uint64_t local_apic_addr;
} __attribute__((packed));

void ACPIManager::ParseMADT() {
  auto* entry = GetEntry("APIC");
  kprintf("entry : %lx ", entry);

  uint8_t* data = entry->data;
  uint32_t local_apic_addr = ReadAndAdvance<uint32_t>(data);
  uint32_t flags = ReadAndAdvance<uint32_t>(data);

  kprintf("local : %x flag : %x \n", local_apic_addr, flags);

  uint8_t* data_end = entry->data + (entry->header.len - sizeof(entry->header));
  while (data < data_end) {
    uint8_t entry_type = ReadAndAdvance<uint8_t>(data);

    // Record len is unused.
    ReadAndAdvance<uint8_t>(data);

    switch (entry_type) {
      case 0: {
        ProcessLocalAPIC p = ReadAndAdvance<ProcessLocalAPIC>(data);
        kprintf("[ProcessLocalAPIC] proc id : %d apic id : %d \n",
                p.acpi_processor_id, p.apic_id);
        core_apic_ids_.push_back(p.apic_id);
        break;
      }
      case 1: {
        IOAPIC p = ReadAndAdvance<IOAPIC>(data);
        kprintf("[IOAPIC] ioapic id : %d o apic addr id : %x %x\n", p.io_apic_id,
                p.io_apic_addr, p.global_sys_intr_base);
        break;
      }
      case 2: {
        InterruptSourceOverride p =
            ReadAndAdvance<InterruptSourceOverride>(data);
        kprintf(
            "[InterruptSourceOverride] bus src : %d irq src: %d glboal sys "
            "intr %x \n",
            p.bus_src, p.irq_src, p.global_sys_intr);
        break;
      }
      case 4: {
        NonMaskableInterrupts p = ReadAndAdvance<NonMaskableInterrupts>(data);
        kprintf("[NonMaskableInterrupts] acpi proc id : %d lint_num: %d \n",
                p.acpi_processor_id, p.lint_num);
        break;
      }
      case 5: {
        LocalAPICAddressOverride p =
            ReadAndAdvance<LocalAPICAddressOverride>(data);
        kprintf("[LocalAPICAddressOverride] acpi local id : %d \n",
                p.local_apic_addr);
        break;
      }
    }
  }
}

struct FADT {
  uint32_t firmware_ctrl;
  uint32_t dsdt;
  uint8_t reserved;
  uint8_t preferred_power_management_profile;
  uint16_t sci_intr;
  uint32_t smi_cmd_port;
  uint8_t acpi_enable;
  uint8_t acpi_disable;
  uint8_t s4_bios_req;
  uint8_t pstate_cntrl;
  uint32_t pm1a_event_block;
  uint32_t pm1b_event_block;
  uint32_t pm1a_control_block;
} __attribute__((packed));

void ACPIManager::EnableACPI() {
  auto* entry = GetEntry("FACP");
  kprintf("entry : %lx ", entry);

  uint8_t* data = entry->data;
  FADT fadt = ReadAndAdvance<FADT>(data);

  outb(fadt.smi_cmd_port, fadt.acpi_enable);

  while ((inw(fadt.pm1a_control_block) & 1) == 0) {
  }
}

}  // namespace Kernel
