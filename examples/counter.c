#include "coroutine.h"

static void counter(void *arg)
{
    size_t n = (size_t) arg;
    for (size_t i = 0; i < n; i++) {
        printf("[%zu] %zu\n", current_co->id, i);
        coroutine_yield();
    }

    coroutine_finish();
}

int main(void)
{
    coroutine_init();
    coroutine_create(counter, (void *) 10);
    coroutine_create(counter, (void *) 5);
    coroutine_create(counter, (void *) 6);
    coroutine_create(counter, (void *) 2);

    coroutine_auto_resume();
    coroutine_auto_collect();

    printf("count: %zu\n", coroutines.count);

    coroutine_destroy();

    return 0;
}

