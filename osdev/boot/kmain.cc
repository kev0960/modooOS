#include "ata.h"
#include "ext2.h"
#include "interrupt.h"
#include "kernel_test.h"
#include "kthread.h"
#include "printf.h"
#include "sync.h"
#include "vga_output.h"

Kernel::VGAOutput<> Kernel::vga_output{};

extern "C" void KernelMain(void);

Kernel::SpinLock m_;
int compute = 0;

void DoSth() {
  int total = 0;
  while (total < 2000) {
    m_.lock();
    Kernel::vga_output << "Done1 ----------" << total << "\n";
    m_.unlock();
    asm volatile("int $0x30");
    total++;
  }
}

void DoSthElse() {
  int total = 0;
  while (total < 1000) {
    m_.lock();
    Kernel::vga_output << "Done2 ----------------- " << total << "\n";
    m_.unlock();
    asm volatile("int $0x30");
    total++;
  }
}

uint64_t total_sum = 0;
void Sum() {
  uint64_t total = 0;
  while (total < 10000000000ull) {
    m_.lock();
    total_sum++;
    m_.unlock();

    total++;
  }
}

void __attribute__((optimize("O0"))) crazy() {
  int total_sum = 0;
  for (int i = 0; i < 100000000; i++) {
    total_sum++;
  }
}

void KernelMain() {
  // Initialize Interrupts.
  Kernel::IDTManager idt_manager{};
  idt_manager.InitializeIDTForCPUException();
  idt_manager.InitializeIDTForIRQ();
  idt_manager.InitializeCustomInterrupt();
  idt_manager.LoadIDT();

  Kernel::vga_output << "IDT setup is done! \n";

  Kernel::KernelThread::InitThread();
  Kernel::vga_output << "Init kThread is done! \n";

  Kernel::kernel_test::KernelTestRunner::GetTestRunner().RunTest();

  auto& ata_driver = Kernel::ATADriver::GetATADriver();
  (void)(ata_driver);

  auto& ext2 = Kernel::Ext2FileSystem::GetExt2FileSystem();
  (void)(ext2);
  /*
  Kernel::KernelThread thread(DoSth);
  thread.Start();

  Kernel::KernelThread thread2(DoSthElse);
  thread2.Start();

  thread.Join();
  thread2.Join();*/

  /*
  asm volatile ("int $10");
  asm volatile ("int $11");
  asm volatile ("int $12");*/
  // int total = 0;
  while (1) {
  }
}
