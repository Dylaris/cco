#include "coroutine.h"

void *counter(void *arg)
{
    (void) arg;
    int n = 4;
    for (int i = 0; i < n; i++) {
        printf("[%zu] %d\n", coroutines.cur_co_id, i);
        coroutine_yield(NULL);
    }

    coroutine_finish();
    return NULL;
}

int main(void)
{
    coroutine_init();
    struct Coroutine *co1 = coroutine_create(counter);
    struct Coroutine *co2 = coroutine_create(counter);
    struct Coroutine *co3 = coroutine_create(counter);

    int dead = 0;
    while (1) {
        coroutine_resume(co1); dead += (co1->status == DEAD) ? 1 : 0;
        coroutine_resume(co2); dead += (co2->status == DEAD) ? 1 : 0;
        coroutine_resume(co3); dead += (co3->status == DEAD) ? 1 : 0;

        if (dead == 3) break;
    }

    printf("well done\n");
    coroutines_destroy();

    return 0;
}

