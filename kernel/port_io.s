.global port_in
.global port_out
port_in:
    mov %di, %dx
    in %dx, %eax
    ret
port_out:
    mov %di, %dx
    mov %esi, %eax 
    out %eax, %dx
    ret