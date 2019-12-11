#ifndef ATA_H
#define ATA_H

#include "filesystem.h"
#include "kthread.h"
#include "types.h"

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
  class ATAFileSystem : public FileSystem {};

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

 private:
  ATADriver() : disk_access_(1) { InitATA(); }
  void InitATA();

  ATADevice primary_master_;
  ATADevice primary_slave_;
  ATADevice secondary_master_;
  ATADevice secondary_slave_;

  // Semaphore to control Disk access.
  Semaphore disk_access_;
};

// Semaphore to send command to Disk.
extern Semaphore kATADiskCommandSema;

};  // namespace Kernel

#endif
