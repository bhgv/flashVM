/* Copyright (c) 2026 bhgv
 * EN: Linux-only failure injection for real runtime pthread creation and joins.
 * RU: Linux-перехватчик настоящих запусков и присоединений потоков VM.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long attempts; /* Atomic creation ordinal, including GC helpers. */
static unsigned long started;  /* Successful real pthread_create calls. */
static unsigned long joined;   /* Successful real pthread_join calls. */
static unsigned long injected; /* Number of deliberately refused creation attempts. */


/* Interpose only this test process. Arguments/ownership follow pthread_create.
 * DVM_TEST_THREAD_FAIL selects one creation ordinal; return EAGAIN there without
 * creating a thread. Other calls forward unchanged. No VM/managed state is read.
 * RU: Поток и аргументы соответствуют pthread_create. DVM_TEST_THREAD_FAIL
 * задаёт номер отказа EAGAIN без создания потока. Остальные вызовы передаются
 * pthread без изменений; управляемые данные не читаются. */
int pthread_create(pthread_t *thread, const pthread_attr_t *attributes,
    void *(*entry)(void *), void *argument)
{
    int (*create)(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);
    void *symbol = dlsym(RTLD_NEXT, "pthread_create");
    const char *selection = getenv("DVM_TEST_THREAD_FAIL");
    unsigned long ordinal = __sync_add_and_fetch(&attempts, 1);
    int result;
    if (selection && ordinal == strtoul(selection, NULL, 10))
    {
        __sync_add_and_fetch(&injected, 1);
        return EAGAIN;
    }
    if (!symbol || sizeof(create) != sizeof(symbol))
    {
        return ENOSYS;
    }
    memcpy(&create, &symbol, sizeof(create));
    result = create(thread, attributes, entry, argument);
    if (!result)
    {
        __sync_add_and_fetch(&started, 1);
    }
    return result;
}


/* Forward the real join and count only successful retirement. thread/result
 * follow pthread_join; counters are native atomics and outlive all helper joins.
 * RU: Передать thread/result настоящему pthread_join, вернуть его результат.
 * Считать только успешные join нативными атомарными счётчиками. */
int pthread_join(pthread_t thread, void **result)
{
    int (*join)(pthread_t, void **);
    void *symbol = dlsym(RTLD_NEXT, "pthread_join");
    int status;
    if (!symbol || sizeof(join) != sizeof(symbol))
    {
        return ENOSYS;
    }
    memcpy(&join, &symbol, sizeof(join));
    status = join(thread, result);
    if (!status)
    {
        __sync_add_and_fetch(&joined, 1);
    }
    return status;
}


/* Emit native lifecycle evidence after main/runtime teardown. No arguments or
 * managed references; the driver rejects missing injection or unpaired joins.
 * RU: После завершения main вывести нативные счётчики. Аргументов и ссылок
 * на управляемую память нет. Драйвер проверяет отказ и парность join. */
__attribute__((destructor)) static void report(void)
{
    fprintf(stderr, "POOL_START_CONTROL started=%lu joined=%lu injected=%lu\n",
        __sync_add_and_fetch(&started, 0), __sync_add_and_fetch(&joined, 0),
        __sync_add_and_fetch(&injected, 0));
}
