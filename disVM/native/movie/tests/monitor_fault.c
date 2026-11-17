/* Copyright (c) 2026 bhgv
 * EN: Owner-local native thread-start failure for autonomous-reader rollback acceptance.
 * RU: Owner-local native thread-start failure для проверки autonomous-reader rollback.
 */
#include <pthread.h>
#include <errno.h>
static __thread int fail_create; /* Only the calling fixture owner can inject one refused start. */
int __real_pthread_create(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);


/* EN/RU: Refuse this native owner's next pthread_create; no other thread is paused or modified.
 * Отказать следующему pthread_create этого owner; другие threads не останавливаются. */
void movie_test_fail_thread_once(void)
{
    fail_create = 1;
}


/* EN/RU: Inject EAGAIN once, otherwise preserve exact pthread_create inputs/results and ownership.
 * Однократно EAGAIN; иначе сохранить inputs/results/ownership pthread_create. */
int __wrap_pthread_create(pthread_t *thread, const pthread_attr_t *attributes,
    void *(*entry)(void *), void *state)
{
    if (fail_create)
    {
        fail_create = 0;
        return EAGAIN;
    }
    return __real_pthread_create(thread, attributes, entry, state);
}
