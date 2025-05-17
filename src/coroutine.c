#include "coroutine.h"

#define ZD_STATIC
#define ZD_IMPLEMENTATION
#define ZD_DS_DYNAMIC_ARRAY
#define ZD_DS_STACK
#include "zd.h"

static struct zd_dyna coroutines = {0};
static struct zd_stack back_stk = {0};   /* record the back address to resume() invoke */
static struct coroutine *current_co = NULL;

static void clear_coroutine(void *arg)
{
    struct coroutine *co = (struct coroutine *) arg;
    if (co->stack_base != NULL) free(co->stack_base);
}

void coroutine_init(void)
{
    zd_dyna_init(&coroutines, sizeof(struct coroutine));
    zd_stack_init(&back_stk, sizeof(struct coroutine *));

    struct coroutine co = {0};
    co.state = ST_RUNNING;
    co.id = coroutines.count;

    zd_dyna_append(&coroutines, &co);

    current_co = (struct coroutine *) zd_dyna_get(&coroutines, coroutines.count - 1);
}

void coroutine_destroy(void)
{
    current_co = NULL;
    zd_stack_destroy(&back_stk, NULL);
    zd_dyna_destroy(&coroutines, clear_coroutine);
}

struct coroutine *coroutine_create(void (*task)(void *), void *arg)
{
    struct coroutine co = {0};
    co.stack_size = COROUTINE_STACK_SIZE;
    co.stack_base = malloc(COROUTINE_STACK_SIZE);
    assert(co.stack_base != NULL);
    co.regs[CTX_RET] = task;
    co.regs[CTX_RSP] = (char *) co.stack_base + co.stack_size - PROTECTION_REGION;     /* reserve some bytes for protection */
    co.regs[CTX_RDI] = arg;
    co.state = ST_READY;
    co.id = coroutines.count;

    zd_dyna_append(&coroutines, &co);

    return zd_dyna_get(&coroutines, coroutines.count - 1);
}

void coroutine_resume(struct coroutine *next)
{
    if (next->state != ST_SUSPEND && next->state != ST_READY) return;

    zd_stack_push(&back_stk, &current_co);
    current_co->state = ST_SUSPEND;

    struct coroutine *cur = current_co;
    current_co = next;
    current_co->state = ST_RUNNING;

    coroutine_switch_context(cur, next);
}

void coroutine_auto_resume(void)
{
    size_t dead = 0;
    while (1) {
        struct coroutine *co_iter = zd_dyna_next(&coroutines);
        if (co_iter == NULL || co_iter->state == ST_DEAD) continue;

        coroutine_resume(co_iter);

        if (co_iter->state == ST_DEAD) dead += 1;
        if (dead == coroutines.count - 1) break;
    }
}

void coroutine_yield(void)
{
    if (current_co->state != ST_RUNNING) return;

    current_co->state = ST_SUSPEND;
    struct coroutine *cur = current_co;

    struct coroutine *next = *(struct coroutine **) zd_stack_pop(&back_stk, NULL);
    assert(next != NULL);
    current_co = next;
    current_co->state = ST_RUNNING;

    coroutine_switch_context(cur, next);
}

void coroutine_finish(void)
{
    current_co->state = ST_DEAD;
    struct coroutine *cur = current_co;

    struct coroutine *next = *(struct coroutine **) zd_stack_pop(&back_stk, NULL);
    assert(next != NULL);
    current_co = next;
    current_co->state = ST_RUNNING;

    coroutine_switch_context(cur, next);
}

void coroutine_collect(struct coroutine *co)
{
    if (co->state != ST_DEAD) return;
    size_t off = co - (struct coroutine *) coroutines.base;
    if (off >= coroutines.count) return;
    zd_dyna_remove(&coroutines, off, clear_coroutine);
}

void coroutine_auto_collect(void)
{
    for (size_t i = 0; i < coroutines.count; i++) {
        struct coroutine *co = zd_dyna_get(&coroutines, i);
        if (co->state != ST_DEAD) continue;
        coroutine_collect(co);
        i = 0;  /* re scan because of zd_dyna_remove() change the count of coroutines array */
    }
}

size_t coroutine_alive(void)
{
    struct coroutine *co_iter = NULL;
    size_t alive = 0;

    while ((co_iter = zd_dyna_next(&coroutines)) != NULL)
        if (co_iter->state != ST_DEAD) alive += 1;

    return alive;
}

size_t coroutine_workid(void)
{
    return current_co->id;
}

size_t coroutine_count(void)
{
    return coroutines.count;
}


void __attribute__((naked)) coroutine_switch_context(struct coroutine *cur, struct coroutine *next)
{
    (void) cur;
    (void) next;
    
    __asm__ __volatile__(
        /* store current coroutine's context */
        "leaq (%rsp), %rax\n\t"
        "movq %rax, 104(%rdi)\n\t"
        "movq %rbx, 96(%rdi)\n\t"
        "movq %rcx, 88(%rdi)\n\t"
        "movq %rdx, 80(%rdi)\n\t"
        "movq 0(%rax), %rax\n\t"
        "movq %rax, 72(%rdi)\n\t"
        "movq %rsi, 64(%rdi)\n\t"
        "movq %rdi, 56(%rdi)\n\t"
        "movq %rbp, 48(%rdi)\n\t"
        "movq %r8, 40(%rdi)\n\t"
        "movq %r9, 32(%rdi)\n\t"
        "movq %r12, 24(%rdi)\n\t"
        "movq %r13, 16(%rdi)\n\t"
        "movq %r14, 8(%rdi)\n\t"
        "movq %r15, (%rdi)\n\t"
        "xorq %rax, %rax\n\t"

        /* restore next coroutine's context */
        "movq 48(%rsi), %rbp\n\t"
        "movq 104(%rsi), %rsp\n\t"
        "movq (%rsi), %r15\n\t"
        "movq 8(%rsi), %r14\n\t"
        "movq 16(%rsi), %r13\n\t"
        "movq 24(%rsi), %r12\n\t"
        "movq 32(%rsi), %r9\n\t"
        "movq 40(%rsi), %r8\n\t"
        "movq 56(%rsi), %rdi\n\t"
        "movq 80(%rsi), %rdx\n\t"
        "movq 88(%rsi), %rcx\n\t"
        "movq 96(%rsi), %rbx\n\t"
        "leaq 8(%rsp), %rsp\n\t"
        "pushq 72(%rsi)\n\t"
        "movq 64(%rsi), %rsi\n\t"
        "ret\n\t"
    );
}
