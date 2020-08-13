#include "util.h"

unsigned short conf_addr_addr = 0xcf8;
unsigned short conf_data_addr = 0xcfc;

unsigned int port_in(unsigned short arg1);
void port_out(unsigned short arg1, unsigned int arg2);


union PciConfAddress {
    unsigned int raw;
    struct {
        unsigned int reg_offset : 8;
        unsigned int func_num : 3;
        unsigned int device_num : 5;
        unsigned int bus_num : 8;
        unsigned int reserved : 7;
        unsigned int enable : 1;
    }__attribute((packed));
};

static unsigned int read_pci_conf(unsigned int bus, unsigned int device, unsigned int func, unsigned int offset) {
    union PciConfAddress config_addr;
    config_addr.enable = 1;
    config_addr.reserved = 0;
    config_addr.bus_num = bus;
    config_addr.device_num = device;
    config_addr.func_num = func;
    config_addr.reg_offset = offset;
    port_out(conf_addr_addr, config_addr.raw); //CONFIG_ADDRESSレジスタへ３つ組のアドレスを格納
    unsigned int ret = port_in(conf_data_addr); //CONFIG_DATAレジスタにread
    return ret;
}

static void write_pci_conf(unsigned int bus, unsigned int device, unsigned int func, unsigned int offset, unsigned int data) {
    union PciConfAddress config_addr;
    config_addr.enable = 1;
    config_addr.reserved = 0;
    config_addr.bus_num = bus;
    config_addr.device_num = device;
    config_addr.func_num = func;
    config_addr.reg_offset = offset;
    port_out(conf_addr_addr, config_addr.raw); //CONFIG_ADDRESSレジスタへ３つ組のアドレスを格納
    port_out(conf_data_addr, data);
    return;
}

static unsigned int get_nic_pci_header_type() {
    unsigned int reg3 = read_pci_conf(0,3,0,0x0c); //Qemuのnicのデバイス構成の、バス番号00、デバイス番号03、ファンクション番号00を渡し、03レジスタのoffsetは0cであるため。
    puts("the content of reg3 is");
    puth(reg3,10);
    unsigned int header_type = (reg3 >> 16) & 0b11111111;
    return header_type;
}

void init_nic_pci() {
    unsigned int header_type = get_nic_pci_header_type();

    if (header_type != 0) { //header type が0か確認
        puts("header type is ");
        puth(header_type, 10);
        return;
    }

    unsigned int mask_on_reg2 = 0b10000000000; //bit10だけ1をたてたマスク
    unsigned int low_16bit_slicer = 0b1111111111111111; //これと&を取ると、下位１６ビットが切り出される.statusフィールドをトグルしないためのもの

    unsigned int content_of_reg2 = read_pci_conf(0,3,0,0x04); //もともとregister2に入っていた値を入手
    
    content_of_reg2 = (content_of_reg2 & low_16bit_slicer) | mask_on_reg2; //下位16ビットだけ切り出し、更にbit10を立てることで割り込みを無効化

    write_pci_conf(0,3,0,0x04,content_of_reg2);

    return;
}

unsigned int get_nic_base_address() {
    unsigned int bar; //BARの中身の格納場所
    unsigned int offset; //BARのアドレスのoffsetを格納
    unsigned int lsb_of_bar; //BARの中身のLSB
    unsigned int bar_type;
    unsigned int ret = 0; //最終的な32ビット物理アドレス

    for (unsigned int i = 0; i < 6; i++) {
        offset = 0x10 + i*4; //offsetは0x10,0x14,0x18,0x1c,0x20,0x24を取りうる
        bar = read_pci_conf(0,3,0,offset); //barレジスタの値を取得
        lsb_of_bar = bar & 1; //barのLSB. barの中身が物理アドレスかioアドレスかの判断に使う
        bar_type = (bar >> 1) & 0b11; //barのbit2と1. 32ビット物理アドレスか64ビットかの判断に使う

        if (bar != 0 && lsb_of_bar == 0 && bar_type == 0x00) { //LSB == 0でtypeも0x0ならば32ビット物理アドレス
            ret = bar;
            break;
        }
    }
    
    if(ret == 0) { //32ビット物理アドレスがなかったとき
        puts("base address is not found in 32bit pysical address!\n");
        return 1;
    }
    
    return ret;
}