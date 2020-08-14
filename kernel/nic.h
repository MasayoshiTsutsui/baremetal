#pragma once

void init_nic(unsigned int);

unsigned char send_frame(void *, unsigned short);

unsigned short receive_frame(void *);

