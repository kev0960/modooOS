#include "ata.h"

#include "../../std/algorithm.h"
#include "../../std/printf.h"
#include "../apic.h"
#include "../io.h"
#include "../qemu_log.h"
#include "../sync.h"

namespace Kernel {
namespace {

constexpr uint16_t kPrimaryIOBasePort = 0x1F0;
constexpr uint16_t kSecondaryIOBasePort = 0x170;
constexpr uint16_t kPrimaryControlBasePort = 0x3F6;
constexpr uint16_t kSecondaryControlBasePort = 0x376;

// "IDENTIFY" command.
constexpr uint16_t kIdentifyCommand = 0xEC;

// Status register bits.
constexpr uint8_t kStatusRegERR = 1 << 0;
constexpr uint8_t kStatusRegIDX = 1 << 1;
constexpr uint8_t kStatusRegCORR = 1 << 2;
constexpr uint8_t kStatusRegDRQ = 1 << 3;
constexpr uint8_t kStatusRegSRV = 1 << 4;
constexpr uint8_t kStatusRegDF = 1 << 5;
constexpr uint8_t kStatusRegRDY = 1 << 6;
constexpr uint8_t kStatusRegBSY = 1 << 7;

void InitATADevice(ATADevice* device, bool primary, bool slave) {
  auto io_base = primary ? kPrimaryIOBasePort : kSecondaryIOBasePort;
  auto control_base =
      primary ? kPrimaryControlBasePort : kSecondaryControlBasePort;

  device->data = io_base;
  device->error = io_base + 1;
  device->sector_count = io_base + 2;
  device->sector_num = io_base + 3;
  device->cylinder_low = io_base + 4;
  device->cylinder_high = io_base + 5;
  device->drive = io_base + 6;
  device->status = io_base + 7;

  device->alternate_status = control_base;
  device->device_address = control_base + 1;

  device->slave = slave;
  device->primary = primary;
}

[[maybe_unused]] void Identify(ATADevice* device) {
  uint16_t select_target_drive_port = device->slave ? 0xB0 : 0xA0;

  // Select the target drive.
  outb(device->drive, select_target_drive_port);

  outb(device->lba_low, 0);
  outb(device->lba_mid, 0);
  outb(device->lba_high, 0);

  outb(device->command, kIdentifyCommand);

  // Read status port to see whether the drive exist.
  int status = inb(device->status);

  if (!status) {
    kprintf("Device Does not Exist! \n");
    return;
  }

  // Poll status port until BSY clears.
  while (true) {
    uint8_t current_status = inb(device->status);
    if (!(current_status & kStatusRegBSY)) {
      break;
    }
  }

  if (inb(device->status) & kStatusRegERR) {
    kprintf("The device is disabled :( \n");
    return;
  }

  // Check LBA mid and LBA high to see if they are non zero. If they are non
  // zero, then the drive is not ATA.
  if (inb(device->lba_mid) || inb(device->lba_high)) {
    kprintf("Device is not ATA! \n");
    return;
  }

  // Poll status port until ERR or DRQ sets.
  uint8_t current_status;
  while (true) {
    current_status = inb(device->status);
    if (current_status & kStatusRegDRQ) {
      break;
    }
    if (current_status & kStatusRegERR) {
      kprintf("Error  \n");
      return;
    }
  }

  // Read 256 16 bit values from data port.
  for (int i = 0; i < 256; i++) {
    inw(device->data);
  }
}

void Delay400ns(ATADevice* device) {
  for (int i = 0; i < 4; i++) {
    inb(device->alternate_status);
  }
}

[[maybe_unused]] bool Poll(ATADevice* device) {
  // Wait until BSY clears.
  while (inb(device->status) & kStatusRegBSY) {
    kprintf("wait busy");
  }

  while (true) {
    auto status = inb(device->status);
    if (status & kStatusRegERR) {
      return false;
    }
    if (status & kStatusRegDRQ) {
      return true;
    }
    kprintf("wait 2busy");
  }
}

void ReadOneSector(ATADevice* device, uint32_t lba, uint8_t* buf,
                   size_t read_size) {
  // kprintf("REad wait!! %d \n", KernelThread::CurrentThread()->Id());
  // Reset device if error.
  auto stat = inb(device->status);
  if ((stat & kStatusRegBSY) || (stat & kStatusRegDRQ)) {
    outb(device->device_control, 4);
    outb(device->device_control, 0);
  }

  outb(device->drive, (0xE0 | (device->slave << 4)) | ((lba >> 24) & 0x0F));

  outb(device->feature, 0x00);

  // Lets just read 1 sector.
  outb(device->sector_count, 1);

  outb(device->lba_low, lba);
  outb(device->lba_mid, lba >> 8);
  outb(device->lba_high, lba >> 16);

  outb(device->command, /* Read Sectors */ 0x20);

  ATADriver::GetATADriver().DiskCommandDown();

  for (size_t i = 0; i < read_size / 2; i++) {
    reinterpret_cast<uint16_t*>(buf)[i] = inw(device->data);
  }

  Delay400ns(device);
}

void WriteOneSector(ATADevice* device, uint32_t lba, uint8_t* buf,
                    size_t buffer_size) {
  if (buffer_size > 512) {
    kprintf("Buffer too large :( \n");
    return;
  }

  outb(device->drive, (0xE0 | (device->slave << 4)) | ((lba >> 24) & 0x0F));

  outb(device->feature, 0x00);

  // Lets just read 1 sector.
  outb(device->sector_count, 1);

  outb(device->lba_low, lba);
  outb(device->lba_mid, lba >> 8);
  outb(device->lba_high, lba >> 16);

  outb(device->command, /* Write Sectors */ 0x30);

  Delay400ns(device);

  bool poll_status = Poll(device);
  if (!poll_status) {
    kprintf("Write fail :( \n");
    return;
  }

  for (size_t i = 0; i < buffer_size / 2; i++) {
    outw(device->data, reinterpret_cast<uint16_t*>(buf)[i]);
  }

  outw(device->command, /* flush */ 0xE7);

  // Wait until BSY clears
  while (inb(device->status) & kStatusRegBSY) {
  };

  Delay400ns(device);
  // TODO Is this a correct location? Seems like Read after write only works in
  // this case.
  ATADriver::GetATADriver().DiskCommandDown();
}

}  // namespace

void ATADriver::InitATA() {
  InitATADevice(&primary_master_, /* primary = */ true, /* slave = */ false);
  InitATADevice(&primary_slave_, /* primary = */ true, /* slave = */ true);
  InitATADevice(&secondary_master_, /* primary = */ false, /* slave = */ false);
  InitATADevice(&secondary_slave_, /* primary = */ false, /* slave = */ true);

  kprintf("Check Primary Master ... \n");
}

void ATADriver::Read(uint8_t* buf, size_t buffer_size, size_t lba) {
  if (buffer_size % 2 != 0) {
    kprintf("Read size must be an even number! \n");
    return;
  }

  for (size_t current_read = 0; current_read < buffer_size;
       current_read += 512) {
    size_t num_to_read = min(512ul, buffer_size - current_read);

    ASSERT(CPURegsAccessProvider::IsInterruptEnabled());
    // Disk access must be exclusive.
    DiskAccessDown();

    // Now direct ATA irqs to the core which called ReadFile.
    APICManager::GetAPICManager().RedirectIRQs(
        0xE, CPUContextManager::GetCurrentCPUId());
    APICManager::GetAPICManager().RedirectIRQs(
        0xF, CPUContextManager::GetCurrentCPUId());

    // Even if this is called inside of the interrupt handler, external
    // interrupt should be received.
    ReadOneSector(&primary_master_, lba, buf + current_read, num_to_read);
    DiskAccessUp();

    // kprintf("Read done %d ", KernelThread::CurrentThread()->Id());
    // 1 LBA = 512 bytes
    lba += 1;
  }
}

void ATADriver::Write(uint8_t* buf, size_t buffer_size, size_t lba) {
  if (buffer_size % 2 != 0) {
    kprintf("Write size must be an even number! \n");
    return;
  }

  for (size_t current_write = 0; current_write < buffer_size;
       current_write += 512) {
    size_t num_to_write = min(512ul, buffer_size - current_write);

    // Disk access must be exclusive.
    DiskAccessDown();

    // Now direct ATA irqs to the core which called ReadFile.
    APICManager::GetAPICManager().RedirectIRQs(
        0xE, CPUContextManager::GetCurrentCPUId());
    APICManager::GetAPICManager().RedirectIRQs(
        0xF, CPUContextManager::GetCurrentCPUId());

    WriteOneSector(&primary_master_, lba, buf + current_write, num_to_write);
    DiskAccessUp();

    // 1 LBA = 512 bytes
    lba += 1;
  }
}

}  // namespace Kernel
