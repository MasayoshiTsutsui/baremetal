#include "pm_timer.h"
#include "util.h"

volatile unsigned int *lvt_timer = (unsigned int *)0xfee00320;
volatile unsigned int *initial_count = (unsigned int *)0xfee00380;
volatile unsigned int *current_count = (unsigned int *)0xfee00390;
volatile unsigned int *divide_config = (unsigned int *)0xfee003e0;

unsigned int lapic_timer_freq_khz;

unsigned int measure_lapic_freq_khz() {
    unsigned int max_of_32bit = 0b11111111111111111111111111111111;
    unsigned int contents_of_lvt = *lvt_timer;
    unsigned int single_mode_shifter = 0b11111111111110011111111111111111;
    *lvt_timer = contents_of_lvt & single_mode_shifter; //単発モードへ移行(すみません。コメントアウト英語だとパッと見でわかりづらいのでやっぱ日本語に戻します)

    *initial_count = max_of_32bit; //カウント開始

    pm_timer_wait_millisec(2000);

    unsigned int counts_after_100msec = *current_count;

    unsigned int answer = (max_of_32bit - counts_after_100msec) / 2000;

    lapic_timer_freq_khz = answer;

    *lvt_timer = contents_of_lvt; //一応lvt_timerレジスタをもとの状態に戻しておく

    return answer;
}