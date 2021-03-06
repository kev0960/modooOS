#ifndef ATA_H
#define ATA_H

#include "../../std/types.h"
#include "../kthread.h"
#include "../sync.h"

namespace Kernel {

struct ATADevice {
  // ATA bus I/O port numbers.

  // Below is based on offset from I/O base.
  uint16_t data;
  union {
    uint16_t error;
    uint16_t feature;
  };
  uint16_t sector_count;

  union {
    uint16_t sector_num;
    uint16_t lba_low;
  };
  union {
    uint16_t cylinder_low;
    uint16_t lba_mid;
  };
  union {
    uint16_t cylinder_high;
    uint16_t lba_high;
  };
  union {
    uint16_t drive;
    uint16_t head;
  };
  union {
    uint16_t status;
    uint16_t command;
  };

  // Below is based on offset from Control base.
  union {
    uint16_t alternate_status;
    uint16_t device_control;
  };
  uint16_t device_address;

  bool slave;
  bool primary;
  bool enabled = false;
};

class ATADriver {
 public:
  ATADriver(const ATADriver&) = delete;
  ATADriver operator=(const ATADriver&) = delete;

  static ATADriver& GetATADriver() {
    static ATADriver ata_driver;
    return ata_driver;
  }

  void Read(uint8_t* buf, size_t buffer_size, size_t lba);
  void Write(uint8_t* buf, size_t buffer_size, size_t lba);

  template <typename T>
  void Read(T* t, size_t lba) {
    Read(reinterpret_cast<uint8_t*>(t), sizeof(T), lba);
  }

  template <typename T>
  void Write(const T& t, size_t lba) {
    Write(reinterpret_cast<uint8_t*>(&t), sizeof(T), lba);
  }

  void DiskCommandDown() { disk_command_.Down(); }
  void DiskCommandUp() { disk_command_.Up(); }

 private:
  ATADriver() : disk_access_("ATALock"), disk_command_(0) { InitATA(); }
  void InitATA();

  ATADevice primary_master_;
  ATADevice primary_slave_;
  ATADevice secondary_master_;
  ATADevice secondary_slave_;

  void DiskAccessDown() { disk_access_.lock(); }
  void DiskAccessUp() { disk_access_.unlock(); }

  // Semaphore to control Disk access.
  MultiCoreSpinLock disk_access_;

  // Semaphore to notify that disk command is done.
  Semaphore disk_command_;
};

};  // namespace Kernel

#endif
