#include "hardware.h"
#include "segmentation.h"
#include "util.h"
#include "pm_timer.h"
#include "lapic_timer.h"
#include "interruption.h"
#include "sched.h"
#include "syscall.h"
#include "memory.h"
#include "pci.h"
#include "nic.h"

void start(void *SystemTable __attribute__ ((unused)), struct HardwareInfo *_hardware_info) {
  // From here - Put this part at the top of start() function
  // Do not use _hardware_info directry since this data is located in UEFI-app space
  hardware_info = *_hardware_info;
  init_segmentation();
  // To here - Put this part at the top of start() function
  init_frame_buffer(&(hardware_info.fb));

  init_acpi_pm_timer(hardware_info.rsdp);

  measure_lapic_freq_khz(); //周波数の測定し、参照を可能にする。基本消さない。

  //init_virtual_memory();

  //void *handler;
  //asm volatile ("lea schedule(%%rip), %[handler]":[handler]"=r"(handler));

  //init_intr(); //割り込みの準備。基本消さない。

  //lapic_periodic_exec(1000, handler);

  //init_tasks();

  // Do not delete it!

  init_nic_pci();

  unsigned int base_addr = get_nic_base_address();

  init_nic(base_addr);

  unsigned char str[] = "tsutsui";
  send_frame(str, sizeof(str));

  puts("send completed!\n");


  while(1) {
    volatile unsigned long long j = 100000000;
    while (j--);
    char buf[2048];
    unsigned short len = receive_frame(buf);
    for (int i = 0; i < len; i++) {
      puth(buf[i], 2);
      if ((i + 1) % 16 == 0) puts(" ");
      else if ((i + 1) % 8 == 0) {
        puts(" ");
      }
      if (len > 0) {
        puts(" ");
      }
    }
  }

  while (1);
}
