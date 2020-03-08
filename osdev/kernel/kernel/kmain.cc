#include "../test/kernel_test.h"
#include "./fs/ata.h"
#include "cpu.h"
#include "descriptor_table.h"
#include "./fs/ext2.h"
#include "interrupt.h"
#include "kthread.h"
#include "paging.h"
#include "printf.h"
#include "process.h"
#include "sync.h"
#include "syscall.h"
#include "vga_output.h"

Kernel::VGAOutput<> Kernel::vga_output{};

extern "C" void KernelMain(void);
extern "C" void __cxa_atexit(void) {}

void Idle() { asm volatile("hlt"); }

void KernelMain() {
  // Initialize Interrupts.
  Kernel::IDTManager idt_manager{};
  idt_manager.InitializeIDTForCPUException();
  idt_manager.InitializeIDTForIRQ();
  idt_manager.InitializeCustomInterrupt();
  idt_manager.LoadIDT();
  Kernel::vga_output << "IDT setup is done! \n";

  auto& gdt_table_manager = Kernel::GDTTableManager::GetGDTTableManager();
  gdt_table_manager.SetUpGDTTables();
  Kernel::vga_output << "Resetting GDT is done! \n";

  Kernel::KernelThread::InitThread();
  Kernel::vga_output << "Init kThread is done! \n";

  // Create an identity mapping of kernel VM memory.
  // (0xFFFFFFFF 80000000 ~ maps to 0x0 ~).
  auto& page_table_manager = Kernel::PageTableManager::GetPageTableManager();
  page_table_manager.SetCR3<Kernel::CPURegsAccessProvider>(
      page_table_manager.GetKernelPml4eBaseAddr());
  Kernel::vga_output << "Init Paging is done! \n";
  Kernel::kernel_test::KernelTestRunner::GetTestRunner().RunTest();

  auto& ata_driver = Kernel::ATADriver::GetATADriver();
  (void)(ata_driver);

  auto& ext2 = Kernel::Ext2FileSystem::GetExt2FileSystem();
  (void)(ext2);

  kprintf("Filesystem setup is done! \n");

  auto& syscall_manager = Kernel::SyscallManager::GetSyscallManager();
  UNUSED(syscall_manager);

  kprintf("Syscall handler setup is done! \n");

  /*
  auto& process_manager = Kernel::ProcessManager::GetProcessManager();
  auto* process = process_manager.CreateProcess("/a.out");
  process->Start();
  auto* process2 = process_manager.CreateProcess("/a.out");
  auto* process3 = process_manager.CreateProcess("/a.out");
  auto* process4 = process_manager.CreateProcess("/a.out");
  UNUSED(process2);
  process2->Start();
  process3->Start();
  process4->Start();
  UNUSED(process);
  */
  while (1) {
  }
}
