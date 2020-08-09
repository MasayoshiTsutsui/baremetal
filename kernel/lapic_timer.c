#include "pm_timer.h"
#include "util.h"
#include "interruption.h"

volatile unsigned int *lvt_timer = (unsigned int *)0xfee00320;
volatile unsigned int *initial_count = (unsigned int *)0xfee00380;
volatile unsigned int *current_count = (unsigned int *)0xfee00390;
volatile unsigned int *divide_config = (unsigned int *)0xfee003e0;

unsigned int lapic_timer_freq_khz;

volatile unsigned int *lapic_eoi = (unsigned int *)0xfee000b0;

void (*reserved_callback)(unsigned long long);

unsigned int measure_lapic_freq_khz() {
    unsigned int max_of_32bit = 0b11111111111111111111111111111111;
    unsigned int contents_of_lvt = *lvt_timer;
    unsigned int contents_of_div = *divide_config;
    unsigned int single_mode_shifter = 0b11111111111110011111111111111111;
    unsigned int div_config_1_1 = 0b00000000000000000000000000001011;
    
    *lvt_timer = contents_of_lvt & single_mode_shifter; //単発モードへ移行
    *divide_config = contents_of_div | div_config_1_1; //分周比を1:1に設定. 

    *initial_count = max_of_32bit; //カウント開始

    pm_timer_wait_millisec(2000);

    unsigned int counts_after_2000msec = *current_count;

    unsigned int answer = (max_of_32bit - counts_after_2000msec) / 2000;

    lapic_timer_freq_khz = answer;

    *lvt_timer = contents_of_lvt; //一応lvt_timerレジスタをもとの状態に戻しておく
    //*divide_config = contents_of_div; //divide_confitも同様

    return answer;
}

void lapic_periodic_exec(unsigned int msec, void *callback){
    init_intr();
    reserved_callback = callback;
    unsigned int shift_to_num32_and_orbital = 0b0100000000000100000;
    *lvt_timer = shift_to_num32_and_orbital;

    unsigned int clocks = lapic_timer_freq_khz * msec;

    *initial_count = clocks;

    return;
}

void lapic_set_eoi() {
    *lapic_eoi = 0;
    return;
}

void lapic_intr_handler_internal(unsigned long long arg1) {
    reserved_callback(arg1);
    *lapic_eoi = 0;
    return;
}