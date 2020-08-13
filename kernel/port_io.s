.global port_in
.global port_out
port_in:
    mov %di, %dx
    in %dx, %eax
    ret
port_out:
    mov %di, %ax
    mov %esi, %edx 
    out %ax, %edx
    ret