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

macro pushall {
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
}

macro popall {
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
}

segment readable executable
; text section

; function: print integer
; param: rax stores the print integer
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

; function: print a series of contigunous numbers
counter:
    push rbp
    mov rbp, rsp
    sub rsp, 8

    push rax
    push rcx

    mov rcx, 10
    mov QWORD [rbp-8], 0 
.loop:
    mov rax, QWORD [rbp-8]
    call print_int
    inc QWORD [rbp-8]
    call coroutine_yield
    loop .loop

.exit:
    pop rcx
    pop rax

    add rsp, 8
    pop rbp

    ret

coroutine_yield:
    ; r15 point to the start address of context
    push rax
    push rbx
    mov rax, [cur_coroutine_id]
    mov rbx, 24
    mul rbx
    add rax, coroutines_context
    mov r15, rax
    pop rbx
    pop rax

    ; store the environment
    mov r14, [rsp]
    mov [r15+2*8], r14        ; sotre rip
    add rsp, 8                ; skip the pushed rip
    pushall
    mov [r15+1*8], rsp        ; restore rsp
    mov [r15+0*8], rbp        ; restore rbp

    ; jump back to main coroutine
    mov r15, coroutines_context
    jmp QWORD [r15+2*8]

; function: resume the execution of coroutine
coroutine_resume:
    ; store the main coroutine's environment
    mov r15, coroutines_context
    mov QWORD [r15+0*8], rbp    ; store rbp
    add rsp, 8                  ; skip the pushed rip
    mov QWORD [r15+1*8], rsp    ; store rsp
    mov r14, [rsp-8]
    mov QWORD [r15+2*8], r14    ; sotre rip

    ; go to next coroutine
    inc QWORD [cur_coroutine_id]
    mov r15, [coroutines_count]
    cmp [cur_coroutine_id], r15
    jl .return
    mov QWORD [cur_coroutine_id], 1
    
.return:
    ; r15 point to the start address of context
    mov rax, [cur_coroutine_id]
    mov rbx, 24
    mul rbx
    add rax, coroutines_context
    mov r15, rax

    mov rbp, [r15+0*8]
    mov rsp, [r15+1*8]
    popall
    jmp QWORD [r15+2*8]

; function: set the environment for the coroutine 
; param: r14 stores the address of beginning instruction
coroutine_init:
    ; r15 point to the start address of context
    push rax
    push rbx
    mov rax, [coroutines_count]
    mov rbx, 24
    mul rbx
    add rax, coroutines_context
    mov r15, rax
    pop rbx
    pop rax

    mov QWORD [r15+2*8], r14        ; sotre rip

    mov r14, [stack_end]
    ; for uniform (the first popall in resume)
    mov r13, rsp    ; store old rsp
    mov rsp, r14    ; push all registers to coroutine stack
    pushall
    mov QWORD [r15+1*8], rsp        ; store rsp
    mov rsp, r13    ; restore old rsp

    mov QWORD [r15+0*8], 0          ; store rbp

    mov r15, [stack_end]
    sub r15, COROUTINE_STACK_SIZE
    mov [stack_end], r15

    inc QWORD [coroutines_count]

    ret

entry _start
_start:
    ; main coroutine
    lea r14, [rip]
    call coroutine_init

    mov rcx, 3 
.init_loop:
    lea r14, [counter]
    call coroutine_init
    loop .init_loop

.resume_loop:
    call coroutine_resume
    jmp .resume_loop

    syscall1 SYS_exit, 0

segment readable writeable
; data section
hello_msg: db "HELLO WORLD", 10, 0
hello_msg_len = $-hello_msg
buffer_overflow_msg: db "buffer is overflow", 10, 0
buffer_overflow_msg_len = $-buffer_overflow_msg
stack_end:        dq coroutines_stack+COROUTINE_CAPACITY*COROUTINE_STACK_SIZE
coroutines_count: dq 0
cur_coroutine_id: dq 0

; bss section
coroutines_stack:   rb COROUTINE_CAPACITY*COROUTINE_STACK_SIZE
coroutines_context: rq 3*COROUTINE_CAPACITY     ; rbp, rsp, rip
