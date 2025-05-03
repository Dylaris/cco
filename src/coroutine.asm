format ELF64

public coroutine_init
public coroutine_yield

extrn __coroutine_init
extrn coroutine_store_context

; setjmp:
;     mov %rbx,(%rdi)         /* rdi is jmp_buf, move registers onto it */
;     mov %rbp,8(%rdi)
;     mov %r12,16(%rdi)
;     mov %r13,24(%rdi)
;     mov %r14,32(%rdi)
;     mov %r15,40(%rdi)
;     lea 8(%rsp),%rdx        /* this is our rsp WITHOUT current ret addr */
;     mov %rdx,48(%rdi)
;     mov (%rsp),%rdx         /* save return addr ptr for new rip */
;     mov %rdx,56(%rdi)
;     xor %eax,%eax           /* always return 0 */
;     ret

coroutine_init:
    mov rdi, rsp    
    jmp __coroutine_init

coroutine_yield:
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    mov rdi, rsp
    call coroutine_store_context
    add rsp, 8*6
    ret
