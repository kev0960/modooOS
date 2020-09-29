#include "../test/kernel_test.h"
#include "./fs/ata.h"
#include "./fs/ext2.h"
#include "acpi.h"
#include "apic.h"
#include "console.h"
#include "cpu.h"
#include "cpu_context.h"
#include "descriptor_table.h"
#include "fonts.h"
#include "fpu.h"
#include "graphic.h"
#include "interrupt.h"
#include "kthread.h"
#include "paging.h"
#include "printf.h"
#include "process.h"
#include "qemu_log.h"
#include "scheduler.h"
#include "sync.h"
#include "syscall.h"
#include "timer.h"
#include "vga_output.h"

using namespace Kernel;

MultiCoreSpinLock spin_lock;

extern "C" void KernelMain(void*);
extern "C" void KernelMainForAP(uint32_t, uint32_t);
extern "C" void __cxa_atexit(void) {}

volatile bool io_ready = false;

void Idle() { asm volatile("hlt"); }
void Sleep1() {
  while (true) {
    TimerManager::GetCurrentTimer().Sleep(10);
    auto cpu_id = Kernel::CPUContextManager::GetCPUContextManager()
                      .GetCPUContext()
                      ->cpu_id;
    UNUSED(cpu_id);
    spin_lock.lock();
    kprintf("SLEEEEEEEEEP !!! %d \n", cpu_id);
    spin_lock.unlock();
  }
}

void Sleep2() {
  while (true) {
    TimerManager::GetCurrentTimer().Sleep(300);
    kprintf("Hi (2) ");
  }
}

void Sleep3() {
  while (true) {
    TimerManager::GetCurrentTimer().Sleep(1000);
    kprintf("Hi (3) ");
  }
}

// ----------------------------------
// Kernel Init Orders (for BSP core).
// ----------------------------------
// Set CPU Context for BSP.
// Install IDT.
//   - Install PIC Timer.
//     This will enable scheduling (though no scheduling will happen because the
//     thread queue would be empty).
// Set GDTR and TSS.
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
void KernelMain(void* multiboot_info) {
  QemuSerialLog::Logf("Multiboot info : %lx \n", multiboot_info);
  CPURegsAccessProvider::DisableInterrupt();

  CPUContextManager::GetCPUContextManager().SetCPUContext((uint32_t)0);

  // Initialize Interrupts.
  IDTManager idt_manager{};
  idt_manager.InitializeIDTForCPUException();
  idt_manager.InitializeCustomInterrupt();
  idt_manager.LoadIDT();
  kprintf("IDT setup is done! \n");

  auto& gdt_table_manager = GDTTableManager::GetGDTTableManager();
  gdt_table_manager.SetUpGDTTables();
  kprintf("Resetting GDT is done! \n");

  idt_manager.InitializeIDTForIRQ();

  CPURegsAccessProvider::EnableInterrupt();

  // Create an identity mapping of kernel VM memory.
  // (0xFFFFFFFF 80000000 ~ maps to 0x0 ~).
  auto& page_table_manager = PageTableManager::GetPageTableManager();
  page_table_manager.SetCR3<CPURegsAccessProvider>(
      page_table_manager.GetKernelPml4eBaseAddr());
  kprintf("Init Paging is done! \n");

  ACPIManager::GetACPIManager().DetectRSDP();
  ACPIManager::GetACPIManager().ParseRSDT();
  // ACPIManager::GetACPIManager().ListTables();
  ACPIManager::GetACPIManager().ParseMADT();

  int num_cores = ACPIManager::GetACPIManager().GetCoreAPICIds().size();

  // Create per-core scheduling queue.
  KernelThreadScheduler::GetKernelThreadScheduler().SetCoreCount(num_cores);

  auto& timer_manager = TimerManager::GetTimerManager();

  // Create per-core timers.
  timer_manager.InstallAPICTimer(num_cores);

  KernelThread::InitThread();
  kprintf("Init kThread is done! \n");

  kernel_test::KernelTestRunner::GetTestRunner().RunTest();

  auto& syscall_manager = SyscallManager::GetSyscallManager();
  syscall_manager.InitSyscall();
  kprintf("Syscall handler setup is done! \n");

  timer_manager.RegisterAlarmClock();
  kprintf("Timer handler is registered \n");

  ACPIManager::GetACPIManager().EnableACPI();
  ACPIManager::GetACPIManager().ParseHPET();

  auto& apic_manager = APICManager::GetAPICManager();
  apic_manager.InitLocalAPIC();

  apic_manager.SendWakeUpAllCores();

  idt_manager.DisablePIC();
  timer_manager.StartAPICTimer();

  /*
  KernelThread thread1(Sleep1);
  KernelThread thread2(Sleep2);
  KernelThread thread3(Sleep3);

  thread1.Start();
  thread2.Start();
  thread3.Start();

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

  timer_manager.Calibrate();

  auto& ata_driver = ATADriver::GetATADriver();
  (void)(ata_driver);

  auto& ext2 = Ext2FileSystem::GetExt2FileSystem();
  (void)(ext2);

  io_ready = true;

  kprintf("Filesystem setup is done! \n");

  // Initialize FPU
  FPUManager::GetFPUManager().InitFPU();

  ParseMultibootInfo(multiboot_info);

  auto& font_manager = FontManager::GetFontManager();
  font_manager.Init();

  /*
  auto& graphic_manager = GraphicManager::GetGraphicManager();
  for (int i = 0; i < 2500; i++) {
    graphic_manager.PutChar('A');
  }
  for (int i = 0; i < 2000; i++) {
    graphic_manager.PutChar('B');
    TimerManager::GetTimerManager().GetCurrentTimer().SleepMs(50);
  }
  */
  // graphic_manager.SyncScreen();
  // font_manager.Draw('A', 0, 20);
  // font_manager.Draw('B', 0, 40);
  // font_manager.Draw('C', 0, 60);

  /*
  auto& process_manager = ProcessManager::GetProcessManager();
  auto* process = process_manager.CreateProcess("/hello");
  process->Start();
  */
  KernelConsole::InitKernelConsole();
  // VGAOutput::GetVGAOutput().ClearScreen();

  KernelConsole::GetKernelConsole().ShowWelcome();

  while (1) {
    KernelThreadScheduler::GetKernelThreadScheduler().Yield();
  }
}

void KernelMainForAP(uint32_t cpu_context_lo, uint32_t cpu_context_hi) {
  CPUContext* context = reinterpret_cast<CPUContext*>(
      ((uint64_t)cpu_context_hi << 32) | cpu_context_lo);
  auto& cpu_context_manager = CPUContextManager::GetCPUContextManager();
  cpu_context_manager.SetCPUContext(context);

  CPURegsAccessProvider::DisableInterrupt();
  IDTManager idt_manager{};
  idt_manager.LoadIDT();
  CPURegsAccessProvider::EnableInterrupt();

  auto& gdt_table_manager = GDTTableManager::GetGDTTableManager();
  gdt_table_manager.SetUpGDTTables();

  APICManager::GetAPICManager().InitLocalAPICForAPs();

  context->ap_boot_done = true;
  kprintf(">>>> cpu id : %d done <<<< \n", context->cpu_id);

  // Wait until every other cores to wake up.
  while (!APICManager::GetAPICManager().IsMulticoreEnabled()) {
  }

  KernelThread::InitThread();
  auto& timer_manager = TimerManager::GetTimerManager();

  // You have to install APIC Timer after waking all other cores. Otherwise it
  // won't send back EOI properly once receiving the timer interrupt.
  timer_manager.StartAPICTimer();
  timer_manager.RegisterAlarmClock();

  while (!io_ready) {
  }
  /*
  if (context->cpu_id > 8) {
    KernelThread thread2(Sleep2);
    thread2.Start();
    thread2.Join();
  }
  KernelThread thread1(Weird);
  KernelThread thread2(Sleep1);
  thread1.Start();
  thread2.Start();
  */
  // thread1.Join();

  // Initialize FPU
  FPUManager::GetFPUManager().InitFPU();

  auto& syscall_manager = SyscallManager::GetSyscallManager();
  syscall_manager.InitSyscall();

  /*
  auto& process_manager = ProcessManager::GetProcessManager();
  auto* process = process_manager.CreateProcess("/hello");
  process->Start();

  auto* process2 = process_manager.CreateProcess("/a.out");
  process2->Start();
  */
  // volatile uint64_t k = 0;
  while (1) {
    KernelThreadScheduler::GetKernelThreadScheduler().Yield();
    // void* data = kmalloc(1 << 12);
    /*
    kprintf("CPU [%d] %lx %lx \n", CPUContextManager::GetCurrentCPUId(), addr,
            addr2);
    kprintf("Thread : (%d) [%x] [%x]\n",
            cpu_context_manager.GetCPUContext()->cpu_id,
            APICManager::GetAPICManager().ReadRegister(0x390),
            APICManager::GetAPICManager().ReadRegister(0x80));*/
    // kfree(data);
    // kprintf("[%d] ", cpu_context_manager.GetCPUContext()->cpu_id);
  }
}

