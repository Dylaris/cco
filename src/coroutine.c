#include "coroutine.h"

#define ZD_IMPLEMENTATION
#include "zd.h"

struct zd_dyna coroutines = {0};
struct zd_stack back_stk = {0};   /* record the back address to resume() invoke */
struct coroutine *current_co = NULL;

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
