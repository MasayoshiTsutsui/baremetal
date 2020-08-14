
#define TX_DESCRIPTORS_NUM 8


struct TxDescriptor {
    unsigned long long buffer_addr;
    unsigned int length : 16;
    unsigned int cso : 8;
    unsigned int cmd : 8;
    unsigned int sta : 4;
    unsigned int reserved : 4;
    unsigned int css : 8;
    unsigned int special : 16;
}__attribute__((packed));

static unsigned int nic_base_address;

static struct TxDescriptor tx_descriptors[TX_DESCRIPTORS_NUM]__attribute__((aligned(16)));

static unsigned int tx_current_idx;

static void set_nic_register(unsigned short offset, unsigned int value) {
    unsigned int *target_reg_addr = nic_base_address + offset;
    *target_reg_addr = value;
    return;
}

static void init_tx() {
    struct TxDescriptor initial_entry;
    initial_entry.buffer_addr = 0;
    initial_entry.special = 0;
    initial_entry.css = 0;
    initial_entry.reserved = 0;
    initial_entry.sta = 0;
    initial_entry.cmd = 0b1001; //rs と eopビットだけ立てる
    initial_entry.cso = 0;
    initial_entry.length = 0;

    for (unsigned int i=0; i < TX_DESCRIPTORS_NUM; i++) {
        tx_descriptors[i] = initial_entry; 
    }//ring bufferの初期化完了

    tx_current_idx = 0; //ring bufferのtail初期化

    unsigned int tx_descriptors_hi = (unsigned int)((unsigned long long)tx_descriptors >> 32); //ring bufferの先頭アドレス上位32ビット
    unsigned int tx_descriptors_lo = (unsigned int)((unsigned long long)tx_descriptors & 0xffffffff); //ring bufferの先頭アドレス下位32ビット
    
    set_nic_register(0x3800, tx_descriptors_lo); //TDBALの方に下位ビットを。
    set_nic_register(0x3804, tx_descriptors_hi); //TDBAHの方に上位ビットを。
    //ring bufferの開始物理アドレスはcpuに伝わった。

    unsigned int ring_buf_len = 16 * TX_DESCRIPTORS_NUM;
    set_nic_register(0x3808, ring_buf_len);
    //ring bufferのサイズは伝わった。

    set_nic_register(0x3810, 0);
    set_nic_register(0x3818, 0);
    //ring bufferのhead tailの初期インデックスが伝わった

    unsigned int config = 0b1010 << 22 | 0x40 << 12 | 0x0f << 4 | 0b1010;
    set_nic_register(0x400, config);
    //送信機能の細かい設定も伝わった。


    return;
}

void init_nic(unsigned int nic_address) {
    nic_base_address = nic_address;

    set_nic_register(0x00d8, 0b11111111111111111); //割り込み全部無効化

    init_tx();
    
    return;
} 

unsigned char send_frame(void *buffer, unsigned short len) {
    tx_descriptors[tx_current_idx].buffer_addr = (unsigned long long)buffer;
    tx_descriptors[tx_current_idx].length = (unsigned int)len;

    tx_current_idx = (tx_current_idx + 1) % TX_DESCRIPTORS_NUM;

    set_nic_register(0x3818, tx_current_idx); //更新されたtailのインデックスをTDTに書き込む。

    while(tx_descriptors[tx_current_idx-1].sta == 0);

    return 0;
}