
typedef enum {
    SYSCALL_PUTS,
} SYSCALL;

unsigned long long syscall_puts(char *str) {
    unsigned long long ans;

    asm volatile(
        "mov %[id], %%rdi\n"
        "mov %[str], %%rsi\n"
        "int $0x80\n"
        "mov %%rax, %[ans]\n"
        :[ans]"=r"(ans)
        :[id]"r"((unsigned long long)SYSCALL_PUTS),
         [str]"m"((unsigned long long)str)
    );
    return ans;
}