#include "../test/kernel_test.h"
#include "./fs/ata.h"
#include "./fs/ext2.h"
#include "acpi.h"
#include "apic.h"
#include "cpu.h"
#include "cpu_context.h"
#include "descriptor_table.h"
#include "interrupt.h"
#include "kthread.h"
#include "paging.h"
#include "printf.h"
#include "process.h"
#include "scheduler.h"
#include "sync.h"
#include "syscall.h"
#include "timer.h"
#include "vga_output.h"

Kernel::VGAOutput<> Kernel::vga_output{};
Kernel::SpinLock spin_lock;

extern "C" void KernelMain(void);
extern "C" void KernelMainForAP(uint32_t, uint32_t);
extern "C" void __cxa_atexit(void) {}

void Idle() { asm volatile("hlt"); }
void Sleep1() {
  while (true) {
    Kernel::pic_timer.Sleep(100);
    Kernel::vga_output << "Hi (1) ";
  }
}

void Sleep2() {
  while (true) {
    Kernel::pic_timer.Sleep(300);
    Kernel::vga_output << "Hi (2) ";
  }
}

void Sleep3() {
  while (true) {
    Kernel::pic_timer.Sleep(1000);
    Kernel::vga_output << "Hi (3) ";
  }
}

void Sleep4() {
  while (true) {
    Kernel::pic_timer.Sleep(100);
    Kernel::vga_output << "Hi (4) ";
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

  auto& gdt_table_manager = Kernel::GDTTableManager::GetGDTTableManager();
  gdt_table_manager.SetUpGDTTables();
  Kernel::vga_output << "Resetting GDT is done! \n";

  Kernel::CPUContextManager::GetCPUContextManager().SetCPUContext((uint32_t)0);
  kprintf(
      "asdf : %lx \n",
      Kernel::CPUContextManager::GetCPUContextManager().GetCPUContext()->self);

  // Create an identity mapping of kernel VM memory.
  // (0xFFFFFFFF 80000000 ~ maps to 0x0 ~).
  auto& page_table_manager = Kernel::PageTableManager::GetPageTableManager();
  page_table_manager.SetCR3<Kernel::CPURegsAccessProvider>(
      page_table_manager.GetKernelPml4eBaseAddr());
  Kernel::vga_output << "Init Paging is done! \n";

  Kernel::ACPIManager::GetACPIManager().DetectRSDP();
  Kernel::ACPIManager::GetACPIManager().ParseRSDT();
  Kernel::ACPIManager::GetACPIManager().ListTables();
  Kernel::ACPIManager::GetACPIManager().ParseMADT();
  Kernel::KernelThreadScheduler::GetKernelThreadScheduler().SetCoreCount(
      Kernel::ACPIManager::GetACPIManager().GetCoreAPICIds().size());

  Kernel::KernelThread::InitThread();
  Kernel::vga_output << "Init kThread is done! \n";

  Kernel::kernel_test::KernelTestRunner::GetTestRunner().RunTest();

  /*
  auto& ata_driver = Kernel::ATADriver::GetATADriver();
  (void)(ata_driver);

  auto& ext2 = Kernel::Ext2FileSystem::GetExt2FileSystem();
  (void)(ext2);
  */
  kprintf("Filesystem setup is done! \n");

  auto& syscall_manager = Kernel::SyscallManager::GetSyscallManager();
  UNUSED(syscall_manager);

  kprintf("Syscall handler setup is done! \n");

  Kernel::pic_timer.RegisterAlarmClock();
  kprintf("Timer handler is registered \n");

  Kernel::ACPIManager::GetACPIManager().EnableACPI();

  Kernel::APICManager::GetAPICManager().InitLocalAPIC();
  idt_manager.DisablePIC();
  Kernel::pic_timer.StartAPICTimer();
  /*
  Kernel::KernelThread thread1(Sleep1);
  Kernel::KernelThread thread2(Sleep2);
  Kernel::KernelThread thread3(Sleep3);
  Kernel::KernelThread thread4(Sleep4);

  thread1.Start();
  thread2.Start();
  thread3.Start();
  thread4.Start();

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
  // auto& cpu_context_manager =
  // Kernel::CPUContextManager::GetCPUContextManager();
  while (1) {
  }
}

void KernelMainForAP(uint32_t cpu_context_lo, uint32_t cpu_context_hi) {
  Kernel::IDTManager idt_manager{};
  idt_manager.LoadIDT();
  // Kernel::vga_output << "IDT setup is done for AP! \n";

  auto& gdt_table_manager = Kernel::GDTTableManager::GetGDTTableManager();
  gdt_table_manager.SetUpGDTTables();
  // Kernel::vga_output << "Resetting GDT is done! \n";
  // Kernel::vga_output << "Init kThread is done! \n";

  Kernel::CPUContext* context = reinterpret_cast<Kernel::CPUContext*>(
      ((uint64_t)cpu_context_hi << 32) | cpu_context_lo);
  kprintf(">>>> cpu id : %d <<<< \n", context->cpu_id);
  Kernel::APICManager::GetAPICManager().InitLocalAPICForAPs();

  auto& cpu_context_manager = Kernel::CPUContextManager::GetCPUContextManager();
  cpu_context_manager.SetCPUContext(context);

  Kernel::KernelThread::InitThread();
  context->ap_boot_done = true;

  Kernel::pic_timer.StartAPICTimer();

  if (context->cpu_id > 8) {
    Kernel::KernelThread thread2(Sleep2);
    thread2.Start();
    thread2.Join();
  }
  Kernel::KernelThread thread1(Sleep1);
  thread1.Start();
  thread1.Join();

  volatile int k = 0;
  while (1) {
    spin_lock.lock();
    // kprintf("Thread : %d \n", cpu_context_manager.GetCPUContext()->cpu_id);
    spin_lock.unlock();
    for (k = 0; k < 10000000; k++) {
    }
    // kprintf("[%d] ", cpu_context_manager.GetCPUContext()->cpu_id);
  }
}
