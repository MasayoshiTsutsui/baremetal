
struct InterruptDescriptor {
    unsigned short offset_lo;
    unsigned short segment;
    unsigned short attribute;
    unsigned short offset_mid;
    unsigned int offset_hi;
    unsigned int reserved;
}__attribute__((packed));

struct InterruptDescriptor IDT[256];

unsigned char data[10];

static void load_Idt_to_Idtr() {
    unsigned short idt_bytes = 16 * 256; //InterruptDescriptorのバイトサイズが16バイト、それが256エントリあ。
    unsigned long long int base_address = IDT; //IDTの先頭アドレスをbase_addressとして保存。これらを、data配列の10バイトの空間にリトルエンディアンで格納.
    data[0] = (idt_bytes - 1) & 255;
    data[1] = (idt_bytes - 1) >> 8;
    data[2] = base_address & 255;
    data[3] = (base_address >> 8) & 255;
    data[4] = (base_address >> 16) & 255;
    data[5] = (base_address >> 24) & 255;
    data[6] = (base_address >> 32) & 255;
    data[7] = (base_address >> 40) & 255;
    data[8] = (base_address >> 48) & 255;
    data[9] = (base_address >> 56) & 255;

    asm volatile ("lidt %0" :: "m"(data)); //idtrレジスタに転送

    return;
}

static void register_intr_handler(unsigned char index, unsigned long long offset, unsigned short segment, unsigned short attribute) {
    struct InterruptDescriptor d;
    unsigned short slice_low_16bit = 0b1111111111111111; //2^16-1。　これと論理積を取ることで、下位16ビットの値を得られる。

    d.offset_lo = offset & slice_low_16bit;
    d.offset_mid = (offset >> 16) & slice_low_16bit;
    d.offset_hi = offset >> 32;

    
    d.segment = segment;
    d.attribute = attribute;

    IDT[index] = d;

    return;
}

void init_intr(){
    load_Idt_to_Idtr();

    //IDTzeroクリア
    struct InterruptDescriptor empty_d;
    empty_d.offset_lo = 0;
    empty_d.segment = 0;
    empty_d.attribute = 0;
    empty_d.offset_mid = 0;
    empty_d.offset_hi = 0;
    empty_d.reserved = 0;

    for (int i=0; i < 256; i++) {
        IDT[i] = empty_d;
    }
    //zeroクリア完了


    //lapic_intr_handlerのidtへの登録
    unsigned char index = 32;
    unsigned long long handler;
    asm volatile ("lea lapic_intr_handler(%%rip), %[handler]" : [handler]"=r"(handler));
    unsigned long long offset = handler;
    unsigned short segment;
    asm volatile ("mov %%cs, %0" : "=r"(segment));
    unsigned short attribute = 0b1000111000000000; //割り込みディスクリプタの権限レベルはひとまず00に。

    register_intr_handler(index, offset, segment, attribute);
    //完了


    //syscall_intr_handlerのidtへの登録
    index = 0x80;
    asm volatile ("lea syscall_handler(%%rip), %[handler]":[handler]"=r"(handler));
    offset = handler;
    register_intr_handler(index, offset, segment, attribute);
    //完了

    //割り込み受付開始
    asm volatile ("sti");

    return;
}
