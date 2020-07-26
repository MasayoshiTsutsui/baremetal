#include "util.h"
#include "lapic_timer.h"

#define TASK_NUM 3
#define STACK_SIZE 4096

char stack0 [STACK_SIZE];
char stack1 [STACK_SIZE];
char stack2 [STACK_SIZE];

static void task0() {
    while (1) {
        puts("hello from task0\r\n");
        volatile int i= 100000000;
        while(i--);
    }
}
static void task1() {
    while (1) {
        puts("hello from task1\r\n");
        volatile int i= 100000000;
        while(i--);
    }
}
static void task2() {
    while (1) {
        puts("hello from task2\r\n");
        volatile int i= 100000000;
        while(i--);
    }
}

struct Task {
    unsigned long long sp;
};

struct Task tasks[TASK_NUM];


static void init_task(int idx, unsigned char *stack_bottom, unsigned long long rip) {
    unsigned long long *sp = (unsigned long long *)stack_bottom;
    unsigned long long ss;
    asm volatile("mov %%ss, %0":"=r"(ss));
    unsigned long long rsp = (unsigned long long)stack_bottom; //最初はまだスタックには何も積まれてないから、スタックの底がそのままスタックトップになる。

    unsigned short cs_reg; //csの値をloadする先

    asm volatile (
        "mov %%cs, %0":"=r"(cs_reg)
    );

    unsigned long long cs = (unsigned long long)cs_reg; //最終的には8byteアラインするから

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

    tasks[idx].sp =  (unsigned long long)stack_bottom - 8*20; //今回はエラーコードを積まなくていいので、ss,rsp,rflags,cs,ripの5つと、汎用レジスタ15個分、計8byte * 20 = 160byte分だけ上に伸びた場所を最終的なスタックトップにする。
    
    return;
}

void init_tasks() {
    init_task(1, (unsigned char *)(stack1+STACK_SIZE), (unsigned long long)task1);
    init_task(2, (unsigned char *)(stack2+STACK_SIZE), (unsigned long long)task2);
    unsigned long long sp0 = (unsigned long long)(stack0+STACK_SIZE);
    asm volatile ("mov %0, %%rsp"::"m"(sp0));
    task0();
}

unsigned char current_idx = 0;

unsigned char to_next() { //たった今まで実行していたtaskのインデックスを返し、current_idxには次のインデックスを格納しておく関数。
    unsigned char idx_just_before = current_idx;
    current_idx = (current_idx + 1) % TASK_NUM;
    return idx_just_before;
}

void schedule(unsigned long long sp) {
    unsigned char idx_just_before = to_next(); //たった今まで実行してたタスクの番号
    unsigned char idx_next = (idx_just_before+1) % TASK_NUM; //次に実行するタスクの番号
    tasks[idx_just_before].sp = sp; //今まで実行してたタスクのスタックポインタをtasksに格納されてる構造体に登録
    unsigned long long next_sp = tasks[idx_next].sp;

    lapic_set_eoi();

    asm volatile ("mov %0, %%rsp"::"m"(next_sp));

    asm volatile (
        "pop %r15\n"
        "pop %r14\n"
        "pop %r13\n"
        "pop %r12\n"
        "pop %r11\n"
        "pop %r10\n"
        "pop %r9\n"
        "pop %r8\n"
        "pop %rdi\n"
        "pop %rsi\n"
        "pop %rbp\n"
        "pop %rbx\n"
        "pop %rdx\n"
        "pop %rcx\n"
        "pop %rax\n"
        "iretq\n");
}


































