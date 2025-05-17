#ifndef COROUTINE_H
#define COROUTINE_H

#include <stddef.h>

#define COROUTINE_STACK_SIZE (16*1024)
#define PROTECTION_REGION    64

// 64bit (context.regs[14])
// low
//      | regs[0]:  r15 |
//      | regs[1]:  r14 |
//      | regs[2]:  r13 |
//      | regs[3]:  r12 |
//      | regs[4]:  r9  |
//      | regs[5]:  r8  |
//      | regs[6]:  rbp |
//      | regs[7]:  rdi |
//      | regs[8]:  rsi |
//      | regs[9]:  ret |   // return address
//      | regs[10]: rdx |
//      | regs[11]: rcx |
//      | regs[12]: rbx |
//      | regs[13]: rsp |
// high

enum ctx_reg_idx {
    CTX_R15, CTX_R14, CTX_R13, CTX_R12, CTX_R9,  CTX_R8,  CTX_RBP,
    CTX_RDI, CTX_RSI, CTX_RET, CTX_RDX, CTX_RCX, CTX_RBX, CTX_RSP
};

#define ST_READY   0
#define ST_RUNNING 1
#define ST_SUSPEND 2
#define ST_DEAD    3

struct coroutine {
    /* context */
    void *regs[14];
    size_t stack_size;
    void *stack_base;

    /* ohters */
    size_t state;
    size_t id;
};

void __attribute__((naked)) coroutine_switch_context(struct coroutine *cur, struct coroutine *next);
void coroutine_init(void);
void coroutine_destroy(void);
struct coroutine *coroutine_create(void (*task)(void *), void *arg);
void coroutine_resume(struct coroutine *next);
void coroutine_auto_resume(void);
void coroutine_yield(void);
void coroutine_finish(void);
void coroutine_collect(struct coroutine *co);
void coroutine_auto_collect(void);
size_t coroutine_alive(void);
size_t coroutine_workid(void);
size_t coroutine_count(void);

#endif /* COROUTINE_H */
