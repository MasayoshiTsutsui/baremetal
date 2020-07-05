#include "hardware.h"
#include "util.h"

unsigned int port_in(unsigned short arg1);
void port_out(unsigned short arg1, unsigned short arg2);
//arg1 is the address of the port. arg2 is the data you want to store in the port.
//now you can only hand 2byte data to port_out so if you want to hand 4byte data, replace si and dx with esi and edx in "port_io.s".

const unsigned long long int freq_hz = 3479545;

unsigned short pm_timer_blk;
char pm_timer_is_32;

const char fadt_sig[4] = "FACP";

void init_acpi_pm_timer(struct RSDP *rsdp) {
    //set the pointer to the top of other_tables
    unsigned long long *other_tables = (unsigned long long *)(rsdp->xsdt_address + 36);

    struct SDTH *sdth = (struct SDTH *)*other_tables;

    //find fadt from other_tables
    while (strcmp((const char*)sdth->signature, fadt_sig, 4) != 0) {
        other_tables += 1;
        sdth = (struct SDTH *)*other_tables;
    }

    struct FADT *fadt = (struct FADT *)sdth;
    
    pm_timer_blk = fadt->PM_TMR_BLK;

    //set 0 in pm_timer_is_32 when it is truly 32 and set 1 otherwise.
    if ((fadt->flags >> 8) % 2 == 1) {
        pm_timer_is_32 = 0;
    }
    else {
        pm_timer_is_32 = 1;
    }
    return;
}

void pm_timer_wait_millisec(unsigned int msec) {
    //the number of counts which this function has to wait.
    unsigned int waiting_time_left = freq_hz * msec / 1000; 

    unsigned int bit32 = 4294967295; //2^32-1
    unsigned int bit24 = 16777215; //2^24 - 1

    unsigned int current_counter;//he keeps the latest counter value obtained by polling.
    unsigned int last_counter; //he keeps the second latest counter value obtained by polling.
    unsigned int counter_difference = 0;//he keeps the difference between the counter data obtained just now and that obtained last time.
    last_counter = port_in(pm_timer_blk);

    if (pm_timer_is_32 == 0) {

        while(waiting_time_left > 0) {
            current_counter = port_in(pm_timer_blk);
            if (current_counter < last_counter) {
                counter_difference = bit32 - last_counter + current_counter + 1; //the last 1 is 2^32 - bit32
            }//process in case that counter trancends the max between the time of our pollings.
            else {
                counter_difference = current_counter - last_counter;
            }
            waiting_time_left -= counter_difference;
            last_counter = current_counter;
        }
    }
    else {
        while(waiting_time_left > 0) {
            current_counter = port_in(pm_timer_blk);
            if (current_counter < last_counter) {
                counter_difference = bit24 - last_counter + current_counter + 1;
            }//process in case that counter trancends the max between the time of our pollings
            else {
                counter_difference = current_counter - last_counter;
            }
            waiting_time_left -= counter_difference;
            last_counter = current_counter;
        }
    }
    return;
}
