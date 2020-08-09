#include "sched.h"
#include "util.h"

struct Entry {
    unsigned long long entry;
}__attribute__((packed));

unsigned long long task_cr3s[TASK_NUM];

unsigned long long get_task_cr3s(unsigned int idx) { //カプセル化
    return task_cr3s[idx];
}


unsigned long long kernel_cr3;

unsigned long long get_kernel_cr3() { //カプセル化
    return kernel_cr3;
}

struct Entry PML4s[TASK_NUM][512]__attribute__((aligned(4096)));
struct Entry PDPs[TASK_NUM][512]__attribute__((aligned(4096)));
struct Entry PDs[TASK_NUM][512]__attribute__((aligned(4096)));
struct Entry PTs[TASK_NUM][8][512]__attribute__((aligned(4096)));


struct Entry kernel_PD[512]__attribute__((aligned(4096)));
struct Entry kernel_PTs[8][512]__attribute__((aligned(4096)));

struct Entry io_PD[512]__attribute__((aligned(4096)));
struct Entry fb_PT[512]__attribute__((aligned(4096)));
struct Entry lapic_PT[512]__attribute__((aligned(4096)));

//格納してほしいアドレスを渡すと、エントリにして返す関数
unsigned long long create_entry(unsigned long long physical) {
    unsigned long long bottom_12bits = 0b11;//１ページテーブルエントリ下位12ビットの値を設定。writable, presentビットだけ立てておいた
    return physical + bottom_12bits; //上位12ビットは0, 下位12ビットは上の通り、中40ビットはアドレスの下位12ビットを切り落としたものになっている。アドレスはすべてたかだか52bit長であり、また、ページテーブルは4KiBアラインメントされているので、下位12ビットと上位12ビットはすべて0だという保証があるからこれで問題ないはず
}

void init_virtual_memory() {

    //アドレスのマッピングを行う
    unsigned int i; //appsの番号に対応
    unsigned int j; //P2テーブルのインデックスに対応
    unsigned int k; //P1テーブルのインデックスに対応

    for (i = 0; i < TASK_NUM; i++) {
        task_cr3s[i] = (unsigned long long)PML4s[i];//task_cr3sに各タスクのPML4テーブルの先頭を格納
        PML4s[i][0].entry = create_entry((unsigned long long)PDPs[i]); //appsもfbもlapicもkernelも、仮想アドレス48ビット中上位9bitは0なので、PML4s[_][0]を参照する。よってここにPDPsの先頭アドレスをもったエントリを入れる。
        PDPs[i][1].entry = create_entry((unsigned long long)PDs[i]); //appsに割り当てられる仮想アドレスの上位10~18ビットは1なので、PDsの１番目のエントリを見ることになる。よってここにPDsの先頭アドレスを格納
        PDPs[i][3].entry = create_entry((unsigned long long)io_PD); //fbもlapicも仮想アドレスの上位10~18ビットが3なので。
        PDPs[i][4].entry = create_entry((unsigned long long)kernel_PD); //kernelの仮想アドレスの上位10~18ビットが4なので。

        //まずはappsのページテーブルを埋めていく。
        for (j = 0; j < 8; j++) {
            PDs[i][j].entry = create_entry((unsigned long long)PTs[i][j]); //appsに割り当てられる仮想アドレスの上位19~27ビットは0~7を取りうる(スタックの底がちょうど8)ので、0の場合から7の場合までで計8通りのP1テーブルに繋がなければならない
            for (k = 0; k < 512; k++) {
                PTs[i][j][k].entry = create_entry((j << 21) + (k << 12) + 0x104000000 + i*0x1000000); //iがappの番号-1になっており、app1は0x104000000~0x105000000, app2は0x105000000~...のようになっているので、iの値によってベースとなる物理アドレスを変えているのが後半。前半のシフト演算は、仮想アドレスと物理アドレスの下位24ビットが変わらないことから、下位23~21ビットがjであり、下位20~12ビットがkであることを利用.
            }
        }//完了

        //次はfbとlapicで分かれる。
        io_PD[0].entry = create_entry((unsigned long long)fb_PT); //fbの仮想アドレスの上位19~27ビットは0しか取らないので。
        io_PD[503].entry = create_entry((unsigned long long)lapic_PT); //lapicの仮想アドレスの上位19~27ビットは503しか取らないので。

        //fbから埋めていく
        for (k = 0; k < 512; k++) {
            fb_PT[k].entry = create_entry((3 << 30) + (0 << 21) + (k << 12)); //fbは仮想アドレスと物理アドレスが一致するので、仮想アドレスの下位12ビット以外をそのまま入れれば良い
        }//完了

        //lapicを埋める
        for (k = 0; k < 512; k++) {
            lapic_PT[k].entry = create_entry((3 << 30) + (503 << 21) + (k << 12)); //lapicも仮想アドレスと物理アドレスが一致する上、下位12ビットより上位のビットは空間内で変わらないので1エントリでよい。
        }
        //完了

        //kernelを埋めていく
        for (j = 0; j < 8; j++) {
            kernel_PD[j].entry = create_entry((unsigned long long)kernel_PTs[j]);
            for (k = 0; k < 512; k++) {
                kernel_PTs[j][k].entry = create_entry((4ull << 30) + (j << 21) + (k << 12)); //kernelも仮想アドレスと物理アドレスが一致する。
            }
        }//完了
    }//アドレスのマッピング完了

    asm volatile ("mov %%cr3, %0":"=r"(kernel_cr3));//カーネル空間を作る、現在のPML4の先頭アドレスをkernel_cr3に格納

    return;

}

