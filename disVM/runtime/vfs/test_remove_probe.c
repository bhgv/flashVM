/* Copyright (c) 2026 bhgv
 * EN: Reuse test-only anchor lifetime counters; omit one remove selection for actual decoy deletion.
 * RU: Повторно использовать test-only lifetime counters; один remove направить на настоящий decoy.
 */
#include "test_anchor_probe.c"

static unsigned publication_calls; /* Under inherited probe mutex; exactly four one-shot effects. */
static int remove_omitted;         /* Atomic once-only path fallback; never production state. */
FVM_Result __real_dvm_resource_remove(DVM_ResourcePlan *);


/* EN: CONTROL1 removes one stale display path using real libc removal. The host
 * must observe deletion of its decoy while the held original remains present.
 * RU: CONTROL1 удаляет один stale display path настоящим libc remove. Host обязан
 * увидеть удаление decoy при сохранившемся original. Plan borrowed, no fake success. */
FVM_Result __wrap_dvm_resource_remove(DVM_ResourcePlan *plan)
{
    const char *suffix = "/source-a/victim-0";
    size_t length = strlen(plan->names.native), tail = strlen(suffix);
    int expected = 0;
    const char *publication = strstr(plan->names.native, "/publication-");
    if (publication)
    {
        pthread_mutex_lock(&mutex);
        publication_calls++;
        if (publication_calls > 4)
        {
            fputs("FAIL remove effect replayed during result publication\n", stderr);
            exit(24);
        }
        if (publication_calls == 4)
        {
            fputs("REMOVE_PUBLICATION calls=4\n", stderr);
        }
        pthread_mutex_unlock(&mutex);
    }
    if (CONTROL == 1 && length >= tail && !strcmp(plan->names.native + length - tail, suffix) &&
        __atomic_compare_exchange_n(&remove_omitted, &expected, 1, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED))
    {
        return remove(plan->names.native) ? FVM_ERROR : FVM_OK;
    }
    return __real_dvm_resource_remove(plan);
}
