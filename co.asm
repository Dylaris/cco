format ELF64 executable

; constants
STDIN_FILENO  = 0
STDOUT_FILENO = 1
STDERR_FILENO = 2

COROUTINE_CAPACITY   = 10
COROUTINE_STACK_SIZE = 1024

; syscall number
SYS_write = 1
SYS_exit  = 60

; syscall macro
macro syscall1 syscall_nr, arg1 {
    push rax
    push rdi

    mov rax, syscall_nr
    mov rdi, arg1
    syscall

    pop rax
    pop rdi
}

macro syscall3 syscall_nr, arg1, arg2, arg3 {
    push rax
    push rdi
    push rsi
    push rdx

    mov rax, syscall_nr
    mov rdi, arg1
    mov rsi, arg2
    mov rdx, arg3
    syscall

    pop rdx
    pop rsi
    pop rdi
    pop rax
}

segment readable executable
; text section

; rax: store the print integer
print_int:
    push rbp
    mov rbp, rsp
    sub rsp, 10+2   ; one for terminator, one for newline

    push rcx
    push rdx
    push rbx

    ; rbp point to the end of 12 bytes buffer
    mov BYTE [rbp-1], 0
    mov BYTE [rbp-2], 10
    sub rbp, 3

    xor rdx, rdx
    xor rcx, rcx
    xor rbx, rbx

    mov rcx, 1      ; rcx stores the index of processing digit
    mov rbx, 10     ; rbx stores the divisor (10)
.convert_loop:
    cmp rcx, 10     ; overflow
    jg .overflow
    div rbx
    add dl, '0'     ; convert to string form
    mov [rbp], dl
    cmp rax, 0      ; convert is over
    je .convert_over
    sub rbp, 1
    xor rdx, rdx
    inc rcx
    jmp .convert_loop

.convert_over:
    add rcx, 2
    syscall3 SYS_write, STDOUT_FILENO, rbp, rcx
    jmp .exit

.overflow:
    syscall3 SYS_write, STDERR_FILENO, buffer_overflow_msg, buffer_overflow_msg_len

.exit:
    pop rbx
    pop rdx
    pop rcx

    add rsp, 10+2
    pop rbp

    ret

counter:
    push rbp
    mov rbp, rsp
    sub rsp, 8

    push rax
    push rcx

    mov rcx, 20
    mov QWORD [rbp-8], 20
.loop:
    mov rax, QWORD [rbp-8]
    call print_int
    inc QWORD [rbp-8]
    loop .loop

.exit:
    pop rcx
    pop rax

    add rsp, 8
    pop rbp

    ret

entry _start
_start:
    call counter

    ; syscall3 SYS_write, STDOUT_FILENO, hello_msg, hello_msg_len
    syscall1 SYS_exit, 10

segment readable writeable
; data section

hello_msg: db "HELLO WORLD", 10, 0
hello_msg_len =  $-hello_msg
buffer_overflow_msg: db "buffer is overflow", 10, 0
buffer_overflow_msg_len = $-buffer_overflow_msg

; bss section
coroutines_stack: rb COROUTINE_CAPACITY*COROUTINE_STACK_SIZE
