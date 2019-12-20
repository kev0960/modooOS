#include "ata.h"
#include "ext2.h"
#include "interrupt.h"
#include "../test/kernel_test.h"
#include "kthread.h"
#include "printf.h"
#include "sync.h"
#include "vga_output.h"

Kernel::VGAOutput<> Kernel::vga_output{};

extern "C" void KernelMain(void);
extern "C" void __cxa_atexit(void) {}

void Idle() {
  asm volatile("hlt");
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

  while (1) {
  }
}
