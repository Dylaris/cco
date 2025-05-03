#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
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

struct Context {
    void *stack;
    void *rsp;
    void *rip;
};

struct Contexts {
    struct Context *items;
    size_t count;
    size_t capacity;
    size_t current_id;
};

struct Contexts contexts = {
    .items      = NULL,
    .count      = 0,
    .capacity   = 0,
    .current_id = 0
};

typedef void *(*coroutine_task)(void *);

void coroutine_init(void);
void coroutine_store_context(void *rsp);
void coroutine_create(coroutine_task func);
void coroutine_yield(void);
void coroutine_resume(void);
void coroutine_switch_context(void *rsp);
void __coroutine_init(void *rsp);

int main(void)
{
    coroutine_init();
    coroutine_yield();
    printf("hello world\n");
    return 0;
}

void coroutine_store_context(void *rsp)
{
    long long *old_rsp = contexts.items[contexts.current_id].rsp;
    contexts.items[contexts.current_id].rip = (void *) *(old_rsp - 1);
    contexts.items[contexts.current_id].rsp = rsp;
}

void __coroutine_init(void *rsp)
{
    dynamic_array_append(contexts, ((struct Context) {NULL, rsp, NULL}));
}

void coroutine_create(coroutine_task func)
{
    void *stack = malloc(sizeof(char) * COROUTINE_STACK_SIZE);
    assert(stack != NULL);
    dynamic_array_append(contexts, ((struct Context) {stack, stack, func}));
}

void coroutine_switch_context(void *rsp)
{
    (void) rsp;
}
