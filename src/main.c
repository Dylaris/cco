#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef dynamic_array_append
#define dynamic_array_append(da, elem)                                  \
    do {                                                                \
        while ((da).capacity <= (da).count) (da).capacity += 10;        \
        (da).items = realloc((da).items, sizeof(elem)*(da).capacity);   \
        assert((da).items != NULL);                                     \
        (da).items[(da).count++] = (elem);                              \
    } while (0)
#endif // dynamic_array_append

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

struct Coroutines coroutines = {
    .items     = NULL,
    .count     = 0,
    .capacity  = 0,
    .cur_co_id = 0
};

typedef void *(*coroutine_task)(void *);

struct Coroutine *coroutine_create(coroutine_task func);
void coroutine_init(void);
void coroutine_resume(struct Coroutine *next_co);
void coroutine_yield(struct Coroutine *next_co);
void coroutine_switch_context(void *cur_ctx, void *next_ctx);
void coroutine_finish(void);
void *counter(void *arg);

int main(void)
{
    coroutine_init();
    struct Coroutine *co1 = coroutine_create(counter);
    struct Coroutine *co2 = coroutine_create(counter);

    while (1) {
        coroutine_resume(co1);
        coroutine_resume(co2);

        if (co1->status == DEAD && co2->status == DEAD) break;
    }

    return 0;
}

void func(int arg)
{
    puts("");
    printf("[%zu] %d\n", coroutines.cur_co_id, arg);
}

void *counter(void *arg)
{
    (void) arg;
    int n = 4;
    for (int i = 0; i < n; i++) {
        func(i);
        coroutine_yield(NULL);
    }

    coroutine_finish();
    return NULL;
}

struct Coroutine *coroutine_create(coroutine_task func)
{
    struct Coroutine coroutine;
    memset(&coroutine, 0, sizeof(struct Coroutine));
    coroutine.stack = malloc(sizeof(char) * COROUTINE_STACK_SIZE);
    assert(coroutine.stack != NULL);
    coroutine.regs[CTX_RSP] = (void *) (((unsigned long) coroutine.stack + COROUTINE_STACK_SIZE) & -16LL);
    coroutine.regs[CTX_RET] = func;
    coroutine.status = ALIVE;

    dynamic_array_append(coroutines, coroutine);

    return (coroutines.items + coroutines.count - 1);
}

// if next_co is NULL, go back to next coroutine
void coroutine_resume(struct Coroutine *next_co)
{
    struct Coroutine *cur_co = coroutines.items + coroutines.cur_co_id;
    coroutines.cur_co_id = next_co - coroutines.items;
    next_co = coroutines.items + coroutines.cur_co_id;
    coroutine_switch_context(cur_co->regs, next_co->regs);
}

// if next_co is NULL, go back to main coroutine
void coroutine_yield(struct Coroutine *next_co)
{
    struct Coroutine *cur_co = coroutines.items + coroutines.cur_co_id;
    if (next_co == NULL) {
        coroutines.cur_co_id = 0;
        next_co = coroutines.items + coroutines.cur_co_id;
    } else {
        coroutines.cur_co_id = next_co - coroutines.items;
    }
    coroutine_switch_context(cur_co->regs, next_co->regs);
}

// initialize the main coroutine (allocate a placeholder in the coroutines array)
void coroutine_init(void)
{
    struct Coroutine coroutine;
    memset(&coroutine, 0, sizeof(struct Coroutine));
    dynamic_array_append(coroutines, coroutine);
    coroutines.cur_co_id = 0;

    struct Coroutine *main_co = coroutines.items;
    coroutine_switch_context(main_co->regs, main_co->regs);
}

void coroutine_finish(void)
{
    coroutines.items[coroutines.cur_co_id].status = DEAD;
    coroutine_yield(NULL);
}

