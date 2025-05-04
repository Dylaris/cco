format ELF64

;; 64bit (context.regs[14])
;; low
;;      | regs[0]:  r15 |
;;      | regs[1]:  r14 |
;;      | regs[2]:  r13 |
;;      | regs[3]:  r12 |
;;      | regs[4]:  r8  |
;;      | regs[5]:  r9  |
;;      | regs[6]:  rbp |
;;      | regs[7]:  rdi |
;;      | regs[8]:  rsi |
;;      | regs[9]:  ret |   // return address
;;      | regs[10]: rdx |
;;      | regs[11]: rcx |
;;      | regs[12]: rbx |
;;      | regs[13]: rsp |
;; high

CTX_R15 = 8*0
CTX_R14 = 8*1
CTX_R13 = 8*2
CTX_R12 = 8*3
CTX_R8  = 8*4
CTX_R9  = 8*5
CTX_RBP = 8*6
CTX_RDI = 8*7
CTX_RSI = 8*8
CTX_RET = 8*9
CTX_RDX = 8*10
CTX_RCX = 8*11
CTX_RBX = 8*12
CTX_RSP = 8*13

;; void coroutine_switch_context(void *cur_ctx, void *next_ctx);
public coroutine_switch_context

;; rdi = cur_ctx, rsi = next_ctx
coroutine_switch_context:
    ;; store context
    mov [rdi+CTX_RSP], rsp
    mov [rdi+CTX_RBX], rbx
    mov [rdi+CTX_RCX], rcx
    mov [rdi+CTX_RDX], rdx
    mov rax, [rsp]  ;; point to the top of stack, where stores return address
    mov [rdi+CTX_RET], rax
    mov [rdi+CTX_RSI], rsi
    mov [rdi+CTX_RDI], rdi
    mov [rdi+CTX_RBP], rbp
    mov [rdi+CTX_R8 ], r8
    mov [rdi+CTX_R9 ], r9
    mov [rdi+CTX_R12], r12
    mov [rdi+CTX_R13], r13
    mov [rdi+CTX_R14], r14
    mov [rdi+CTX_R15], r15
    xor rax, rax    ;; always return 0

    ;; restore context
    mov r15, [rsi+CTX_R15]
    mov r14, [rsi+CTX_R14]
    mov r13, [rsi+CTX_R13]
    mov r12, [rsi+CTX_R12]
    mov r9 , [rsi+CTX_R9 ]
    mov r8 , [rsi+CTX_R8 ]
    mov rbp, [rsi+CTX_RBP]
    mov rdi, [rsi+CTX_RDI]
    mov rdx, [rsi+CTX_RDX]
    mov rcx, [rsi+CTX_RCX]
    mov rbx, [rsi+CTX_RBX]
    mov rsp, [rsi+CTX_RSP]
    add rsp, 8
    push QWORD [rsi+CTX_RET]    ;; use ret to flush rip instead of mov
    mov rsi, [rsi+CTX_RSI]

    ret
