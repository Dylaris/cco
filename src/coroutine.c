#include "coroutine.h"

#ifndef dynamic_array
#define dynamic_array_append(da, elem)                                  \
    do {                                                                \
        if ((da).capacity <= (da).count) {                              \
            (da).capacity += 10;                                        \
            (da).items = realloc((da).items,                            \
                    sizeof(elem)*(da).capacity);                        \
            assert((da).items != NULL);                                 \
        }                                                               \
        (da).items[(da).count++] = (elem);                              \
    } while (0)
#define dynamic_array_remove(da, idx, clear_func)                       \
    do {                                                                \
        if ((da).count > 0) {                                           \
            if (clear_func != NULL) clear_func((da).items+idx);         \
            memcpy((da).items+(idx), (da).items+(idx)+1,                \
                    sizeof(*(da).items)*((da).capacity-(idx)-1));       \
            (da).count--;                                               \
        }                                                               \
    } while (0)
#endif // dynamic_array

struct Coroutine _coroutines[10];

struct Coroutines coroutines = {
    // .items     = NULL,
    .items     = _coroutines,
    .count     = 0,
    .capacity  = 0,
    .cur_co_id = 0
};

struct Coroutine *coroutine_create(coroutine_task func)
{
    struct Coroutine coroutine;
    memset(&coroutine, 0, sizeof(struct Coroutine));
    coroutine.stack = malloc(sizeof(char) * COROUTINE_STACK_SIZE);
    assert(coroutine.stack != NULL);
    coroutine.regs[CTX_RSP] = (void *) (((unsigned long) coroutine.stack + COROUTINE_STACK_SIZE) & -16LL);
    coroutine.regs[CTX_RET] = func;
    coroutine.status = ALIVE;

    // dynamic_array_append(coroutines, coroutine);
    coroutines.items[coroutines.count++] = coroutine;

    return (coroutines.items + coroutines.count - 1);
}

// if next_co is NULL, go back to next coroutine
void coroutine_resume(struct Coroutine *next_co)
{
    if (next_co == NULL) return;
    struct Coroutine *cur_co = coroutines.items + coroutines.cur_co_id;
    coroutines.cur_co_id = next_co - coroutines.items;
    coroutine_switch_context(&(cur_co->regs), &(next_co->regs));
}

// if next_co is NULL, go back to main coroutine
void coroutine_yield(struct Coroutine *next_co)
{
    struct Coroutine *cur_co = coroutines.items + coroutines.cur_co_id;
    if (next_co == NULL) {
        coroutines.cur_co_id = 0;
        next_co = coroutines.items;
    } else {
        coroutines.cur_co_id = next_co - coroutines.items;
    }
    coroutine_switch_context(&(cur_co->regs), &(next_co->regs));
}

void coroutines_destroy(void)
{
    // skip the main coroutine
    for (size_t i = 1; i < coroutines.count - 1; i++)
        free(coroutines.items[i].stack);
    // free(coroutines.items);
}

// initialize the main coroutine (allocate a placeholder in the coroutines array)
void coroutine_init(void)
{
    printf("CCO START\n"); // you must have this one to avoid a fucking unknown bug :-)
    
    struct Coroutine coroutine;
    memset(&coroutine, 0, sizeof(struct Coroutine));
    // dynamic_array_append(coroutines, coroutine);
    coroutines.items[coroutines.count++] = coroutine;
    coroutines.cur_co_id = 0;

    struct Coroutine *main_co = coroutines.items;
    coroutine_switch_context(&(main_co->regs), &(main_co->regs));
}

void coroutine_finish(void)
{
    coroutines.items[coroutines.cur_co_id].status = DEAD;
    coroutine_yield(NULL);
}

