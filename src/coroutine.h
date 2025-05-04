#ifndef COROUTINE_H
#define COROUTINE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define COROUTINE_STACK_SIZE (1 << 10)

// 64bit (context.regs[14])
// low
//      | regs[0]:  r15 |
//      | regs[1]:  r14 |
//      | regs[2]:  r13 |
//      | regs[3]:  r12 |
//      | regs[4]:  r8  |
//      | regs[5]:  r9  |
//      | regs[6]:  rbp |
//      | regs[7]:  rdi |
//      | regs[8]:  rsi |
//      | regs[9]:  ret |   // return address
//      | regs[10]: rdx |
//      | regs[11]: rcx |
//      | regs[12]: rbx |
//      | regs[13]: rsp |
// high

enum {
    CTX_RDI = 7,
    CTX_RSI = 8,
    CTX_RET = 9,
    CTX_RSP = 13
};

#define ALIVE 0
#define DEAD  1

struct Coroutine {
    void *regs[14];     // execution environment 
    void *stack;        // start address of private stack
    int status;
};

struct Coroutines {
    struct Coroutine *items;
    size_t count;
    size_t capacity;
    size_t cur_co_id;
};

extern struct Coroutines coroutines;

typedef void *(*coroutine_task)(void *);

void coroutine_init(void);
struct Coroutine *coroutine_create(coroutine_task func);
void coroutine_resume(struct Coroutine *next_co);
void coroutine_yield(struct Coroutine *next_co);
void coroutine_switch_context(void *cur_ctx, void *next_ctx);
void coroutine_finish(void);
void coroutines_destroy(void);

#endif // COROUTINE_H
