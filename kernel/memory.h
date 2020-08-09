#pragma once

unsigned long long get_task_cr3s(unsigned int);

unsigned long long get_kernel_cr3();


void init_virtual_memory();