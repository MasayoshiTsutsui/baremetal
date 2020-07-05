#include "hardware.h"
#include "segmentation.h"
#include "util.h"
#include "pm_timer.h"
#include "lapic_timer.h"

void start(void *SystemTable __attribute__ ((unused)), struct HardwareInfo *_hardware_info) {
  // From here - Put this part at the top of start() function
  // Do not use _hardware_info directry since this data is located in UEFI-app space
  hardware_info = *_hardware_info;
  init_segmentation();
  // To here - Put this part at the top of start() function
  init_frame_buffer(&(hardware_info.fb));

  char *test = "Hello\n";
  puts(test);

  init_acpi_pm_timer(hardware_info.rsdp);
  pm_timer_wait_millisec(3000);

  unsigned long long value = 1985;
  unsigned char digit_len = 3;
  puth(value, digit_len);


  unsigned int frq = measure_lapic_freq_khz();
  puth(frq, 8);

  // Do not delete it!
  while (1);
}
