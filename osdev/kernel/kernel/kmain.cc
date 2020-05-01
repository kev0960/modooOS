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

using namespace Kernel;

Kernel::VGAOutput<> Kernel::vga_output{};
MultiCoreSpinLock spin_lock;

extern "C" void KernelMain(void);
extern "C" void KernelMainForAP(uint32_t, uint32_t);
extern "C" void __cxa_atexit(void) {}

void Idle() { asm volatile("hlt"); }
void Sleep1() {
  while (true) {
    TimerManager::GetCurrentTimer().Sleep(50);
    auto cpu_id = Kernel::CPUContextManager::GetCPUContextManager()
                      .GetCPUContext()
                      ->cpu_id;
    kprintf("", cpu_id);
  }
}

void Sleep2() {
  while (true) {
    TimerManager::GetCurrentTimer().Sleep(300);
    Kernel::vga_output << "Hi (2) ";
  }
}

void Sleep3() {
  while (true) {
    TimerManager::GetCurrentTimer().Sleep(1000);
    Kernel::vga_output << "Hi (3) ";
  }
}

void Sleep4() {
  while (true) {
    TimerManager::GetCurrentTimer().Sleep(100);
    Kernel::vga_output << "Hi (4) ";
  }
}

// ----------------------------------
// Kernel Init Orders (for BSP core).
// ----------------------------------
// Install IDT.
//   - Install PIC Timer.
//     This will enable scheduling (though no scheduling will happen because the
//     thread queue would be empty).
// Set GDTR and TSS.
// Set CPU Context for BSP.
// Set Page table.
// Parse ACPI table.
//   - Get the information about other cores.
// Create per-core scheduling queue.
// Create per-core timer.
// Start threading.
// Install Syscall handler.
// Register alarm clock for BSP.
// Waking up other cores.
// Set up IOAPIC.
// Set up filesystem.
void KernelMain() {
  // Initialize Interrupts.
  IDTManager idt_manager{};
  idt_manager.InitializeIDTForCPUException();
  idt_manager.InitializeCustomInterrupt();
  idt_manager.LoadIDT();
  Kernel::vga_output << "IDT setup is done! \n";

  auto& gdt_table_manager = GDTTableManager::GetGDTTableManager();
  gdt_table_manager.SetUpGDTTables();
  Kernel::vga_output << "Resetting GDT is done! \n";

  CPUContextManager::GetCPUContextManager().SetCPUContext((uint32_t)0);
  idt_manager.InitializeIDTForIRQ();

  // Create an identity mapping of kernel VM memory.
  // (0xFFFFFFFF 80000000 ~ maps to 0x0 ~).
  auto& page_table_manager = PageTableManager::GetPageTableManager();
  page_table_manager.SetCR3<CPURegsAccessProvider>(
      page_table_manager.GetKernelPml4eBaseAddr());
  Kernel::vga_output << "Init Paging is done! \n";

  ACPIManager::GetACPIManager().DetectRSDP();
  ACPIManager::GetACPIManager().ParseRSDT();
  ACPIManager::GetACPIManager().ListTables();
  ACPIManager::GetACPIManager().ParseMADT();

  int num_cores = ACPIManager::GetACPIManager().GetCoreAPICIds().size();

  // Create per-core scheduling queue.
  KernelThreadScheduler::GetKernelThreadScheduler().SetCoreCount(num_cores);

  auto& timer_manager = TimerManager::GetTimerManager();

  // Create per-core timers.
  timer_manager.InstallAPICTimer(num_cores);

  KernelThread::InitThread();
  Kernel::vga_output << "Init kThread is done! \n";

  kernel_test::KernelTestRunner::GetTestRunner().RunTest();

  auto& syscall_manager = SyscallManager::GetSyscallManager();
  UNUSED(syscall_manager);

  kprintf("Syscall handler setup is done! \n");

  timer_manager.RegisterAlarmClock();
  kprintf("Timer handler is registered \n");

  ACPIManager::GetACPIManager().EnableACPI();

  auto& apic_manager = APICManager::GetAPICManager();
  apic_manager.InitLocalAPIC();

  idt_manager.DisablePIC();
  timer_manager.StartAPICTimer();

  /*
  KernelThread thread1(Sleep1);
  KernelThread thread2(Sleep2);
  KernelThread thread3(Sleep3);
  KernelThread thread4(Sleep4);

  thread1.Start();
  thread2.Start();
  thread3.Start();
  thread4.Start();

  auto& process_manager = ProcessManager::GetProcessManager();
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
  // CPUContextManager::GetCPUContextManager();

  apic_manager.InitIOAPIC();
  apic_manager.RedirectIRQs(0x1, 0);
  apic_manager.RedirectIRQs(0xE, 0);
  apic_manager.RedirectIRQs(0xF, 0);

  auto& ata_driver = ATADriver::GetATADriver();
  (void)(ata_driver);

  auto& ext2 = Ext2FileSystem::GetExt2FileSystem();
  (void)(ext2);

  kprintf("Filesystem setup is done! \n");

  /*
  auto& process_manager = ProcessManager::GetProcessManager();
  auto* process = process_manager.CreateProcess("/a.out");
  process->Start();
*/
  while (1) {
  }
}

void KernelMainForAP(uint32_t cpu_context_lo, uint32_t cpu_context_hi) {
  IDTManager idt_manager{};
  idt_manager.LoadIDT();
  // Kernel::vga_output << "IDT setup is done for AP! \n";

  auto& gdt_table_manager = GDTTableManager::GetGDTTableManager();
  gdt_table_manager.SetUpGDTTables();
  // Kernel::vga_output << "Resetting GDT is done! \n";
  // Kernel::vga_output << "Init kThread is done! \n";

  CPUContext* context = reinterpret_cast<CPUContext*>(
      ((uint64_t)cpu_context_hi << 32) | cpu_context_lo);
  kprintf(">>>> cpu id : %d <<<< \n", context->cpu_id);
  APICManager::GetAPICManager().InitLocalAPICForAPs();

  auto& cpu_context_manager = CPUContextManager::GetCPUContextManager();
  cpu_context_manager.SetCPUContext(context);

  context->ap_boot_done = true;

  // kprintf(">>>> cpu id : %d done <<<< \n", context->cpu_id);
  KernelThread::InitThread();
  auto& timer_manager = TimerManager::GetTimerManager();

  // Wait until every other cores to wake up.
  while (!APICManager::GetAPICManager().IsMulticoreEnabled()) {
  }

  // You have to install APIC Timer after waking all other cores. Otherwise it
  // won't send back EOI properly once receiving the timer interrupt.
  timer_manager.StartAPICTimer();
  timer_manager.RegisterAlarmClock();

  /*
  if (context->cpu_id > 8) {
    KernelThread thread2(Sleep2);
    thread2.Start();
    thread2.Join();
  }
  */
  KernelThread thread1(Sleep1);
  thread1.Start();
  // thread1.Join();

  volatile uint64_t k = 0;
  while (1) {
    spin_lock.lock();
    kprintf("Thread : (%d) [%x] [%x]\n",
            cpu_context_manager.GetCPUContext()->cpu_id,
            APICManager::GetAPICManager().ReadRegister(0x390),
            APICManager::GetAPICManager().ReadRegister(0x80));
    spin_lock.unlock();
    for (k = 0; k < 1000; k++) {
    }
    // kprintf("[%d] ", cpu_context_manager.GetCPUContext()->cpu_id);
  }
}
