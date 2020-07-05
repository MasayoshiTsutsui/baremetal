#pragma once

void init_acpi_pm_timer(struct RSDP *);

void pm_timer_wait_millisec(unsigned int);