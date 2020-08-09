#include "util.h"
#include "lapic_timer.h"
#include "memory.h"
#define TASK_NUM 3
#define APP_START 0x40000000
#define APP_END 0x41000000
//#define STACK_SIZE 4096
//char stack0 [STACK_SIZE];
//char stack1 [STACK_SIZE];
//char stack2 [STACK_SIZE];
//static void task0() {
//while (1) {
//puts("hello from task0\r\n");
//volatile int i= 100000000;
//while(i--);
//}
//}
//static void task1() {
//while (1) {
//puts("hello from task1\r\n");
//volatile int i= 100000000;
//while(i--);
//}
//}
//static void task2() {
//while (1) {
//puts("hello from task2\r\n");
//volatile int i= 100000000;
//while(i--);
//}
//}
extern unsigned long long task_cr3s[TASK_NUM];
extern unsigned long long kernel_cr3;
struct Task {
unsigned long long sp;
unsigned long long cr3;
};
struct Task tasks[TASK_NUM];
static void init_task(int idx, unsigned char *stack_bottom, unsigned long long rip) {
unsigned long long task_cr3 = tasks[idx].cr3;
asm volatile("mov %[task_cr3], %%cr3"::[task_cr3]"r"(task_cr3)); //appの仮想アドレス空間に切り替え
unsigned long long *sp = (unsigned long long *)stack_bottom;
unsigned long long ss;
asm volatile("mov %%ss, %0":"=r"(ss));
unsigned long long rsp = (unsigned long long)stack_bottom; //最初はまだスタックには何も積まれてないから、スタックの底がそのままスタックトップになる。
unsigned short cs_reg; //csの値をloadする先
asm volatile (
"mov %%cs, %0":"=r"(cs_reg)
);
unsigned long long cs = (unsigned long long)cs_reg; //最終的には8byte幅にするから
//ここから実際にstackに各レジスタの値を積んでいく
*(sp-1) = ss; //一応、stack_bottomとして渡されるのが,本当にスタックの底(今回で言えば、&stackn[STACK_SIZE]的な(STACK_SIZE-1じゃないことに注意))と想定している
*(sp-2) = rsp;
unsigned long long current_sp = (unsigned long long)stack_bottom - 16; //このアドレスの上にrfalgsの値が積まれるべきなので、これをrspに入れてからpushfqを発行。sp-2と等価。
unsigned long long reg64; //現在のスタックポインタの避難場所
asm volatile (
"mov %%rsp, %0\n"
"mov %1, %%rsp\n"
"pushfq\n":"=r"(reg64):"m"(current_sp)
);
asm volatile ("mov %0, %%rsp"::"m"(reg64)); //ちゃんと元のスタックポインタの値に戻しておく。
*(sp-4) = cs;
*(sp-5) = rip;
//積み上げ完了
asm volatile("mov %[kernel_cr3], %%cr3"::[kernel_cr3]"r"(kernel_cr3)); //kernelの仮想アドレス空間に戻す
return;
}
void init_tasks() {
for (unsigned int i = 0; i < TASK_NUM; i++) {
tasks[i].cr3 = task_cr3s[i];//cr3レジスタの値を登録
tasks[i].sp = APP_END - 8*20;//今回はエラーコードを積まなくていいので、ss,rsp,rflags,cs,ripの5つと、汎用レジスタ15個分、計8byte * 20 = 160byte分だけ上に伸びた場所を最終的なスタックトップにする。
}
puts("before init_task!\n");
init_task(1, (unsigned char *)APP_END, (unsigned long long)APP_START);
init_task(2, (unsigned char *)APP_END, (unsigned long long)APP_START);
puts("after init_task!\n");
unsigned long long sp0 = (unsigned long long)APP_END; //スタックポインタに入れるのは仮想アドレスの方の底
unsigned long long app1_cr3 = task_cr3s[0]; //cr3にはapp1の仮想アドレスを作るcr3の値を入れる
unsigned long long rip = (unsigned long long)APP_START; //jmp先は仮想アドレスの方
asm volatile ("mov %0, %%cr3"::"r"(app1_cr3));
asm volatile ("mov %0, %%rsp"::"r"(sp0));
asm volatile ("jmp *%[app1_adr]"::[app1_adr]"r"(rip));
}
unsigned char current_idx = 0;
unsigned char to_next() { //たった今まで実行していたtaskのインデックスを返し、current_idxには次のインデックスを格納しておく関数。
unsigned char idx_just_before = current_idx;
current_idx = (current_idx + 1) % TASK_NUM;
return idx_just_before;
}
void schedule(unsigned long long sp) {
tasks[current_idx].sp = sp;
current_idx = (++current_idx)%3;
lapic_set_eoi();
asm volatile ("mov %0, %%cr3" :: "r"(task_cr3s[current_idx]));
asm volatile (
"mov %0, %%rsp\n"
"pop %%r15\n"
"pop %%r14\n"
"pop %%r13\n"
"pop %%r12\n"
"pop %%r11\n"
"pop %%r10\n"
"pop %%r9\n"
"pop %%r8\n"
"pop %%rdi\n"
"pop %%rsi\n"
"pop %%rbp\n"
"pop %%rbx\n"
"pop %%rdx\n"
"pop %%rcx\n"
"pop %%rax\n"
"iretq\n" :: "m"(tasks[current_idx].sp)
);
}