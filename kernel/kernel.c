#include "hardware.h"
#include "segmentation.h"
#include "util.h"
#include "pm_timer.h"
#include "lapic_timer.h"
#include "interruption.h"
#include "sched.h"
#include "syscall.h"

void start(void *SystemTable __attribute__ ((unused)), struct HardwareInfo *_hardware_info) {
  // From here - Put this part at the top of start() function
  // Do not use _hardware_info directry since this data is located in UEFI-app space
  hardware_info = *_hardware_info;
  init_segmentation();
  // To here - Put this part at the top of start() function
  init_frame_buffer(&(hardware_info.fb));

  init_acpi_pm_timer(hardware_info.rsdp);

  measure_lapic_freq_khz(); //周波数の測定し、参照を可能にする。基本消さない。


  void *handler;
  asm volatile ("lea schedule(%%rip), %[handler]":[handler]"=r"(handler));

  init_intr(); //割り込みの準備。基本消さない。

  //char *str = "syscall completed!\n";
  //unsigned long long ret;
  //asm volatile("mov %[call_id], %%rdi\n"
               //"mov %[str], %%rsi\n"
               //"int $0x80\n"
               //"mov %%rax, %[ret]\n"
               //:[ret]"=r"(ret)
               //:[call_id]"r"((unsigned long long)SYSCALL_PUTS),
                //[str]"m"((unsigned long long)str)
  //);
  //if (ret == 0) {
    //puts("ret is correct!\n");
  //}

  lapic_periodic_exec(1000, handler);
  init_tasks();

  // Do not delete it!
  while (1);
}
