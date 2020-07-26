#pragma once


unsigned int measure_lapic_freq_khz();

void lapic_periodic_exec(unsigned int, void *);

void lapic_set_eoi();

void lapic_intr_handler_internal();