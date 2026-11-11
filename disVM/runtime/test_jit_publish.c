/* Copyright (c) 2026 bhgv
 * EN: Observe real shared compiler entries and executable mapping ownership.
 * RU: Наблюдать настоящие входы общего компилятора и владение страницами кода.
 */
#include "lib9.h"
#include "interp.h"
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>

static pthread_mutex_t ledger = PTHREAD_MUTEX_INITIALIZER; /* Native test metadata only. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Bounded allocation alignment. */
static _Thread_local int observing; /* Real compilation of the fixture's shared Module. */
static void *pages[8]; /* Actual provider mappings, cleared by their real munmap. */
static unsigned allocated; /* Genuine mappings created, under ledger. */
static unsigned released; /* Genuine mapping retirements, under ledger. */
static unsigned arrived; /* Allocation contenders before code publication. */
static unsigned rejected; /* Exactly one injected native permission failure in CONTROL. */
static unsigned explicit_retries; /* Injected failure occurred during an explicit Loader call. */
static _Thread_local int explicit_call; /* This compilation has an instance argument. */
static unsigned calls; /* Genuine explicit shared compilation calls. */

int __real_compile(Module *, int, Modlink *);
void *__real_mmap64(void *, size_t, int, int, int, off_t);
int __real_munmap(void *, size_t);
int __real_mprotect(void *, size_t, int);
void __real_dvm_close_worker_stop(void);


/* EN: Forward borrowed module/size/instance exactly once; observe only provider.
 * Return real compiler status and leave its ownership/continuation unchanged.
 * RU: Передать module/size/instance один раз; наблюдать только provider.
 * Вернуть настоящий статус без изменения владения/продолжения. */
int __wrap_compile(Module *module, int size, Modlink *instance)
{
    int target = module && module->name && !strcmp(module->name, "JitProvider");
    observing = target;
    explicit_call = target && instance;
    if (target && instance)
    {
        pthread_mutex_lock(&ledger);
        calls++;
        pthread_mutex_unlock(&ledger);
    }
    int result = __real_compile(module, size, instance);
    observing = 0;
    explicit_call = 0;
    return result;
}


/* EN: Align real provider code allocations for at most 250ms, allowing a mutex
 * to correctly admit one builder. Forward address/size/protection/flags/fd/offset.
 * RU: Совместить выделения provider максимум на 250мс, позволяя mutex пропустить
 * одного строителя. Передать address/size/protection/flags/fd/offset без изменений. */
void *__wrap_mmap64(void *address, size_t size, int protection, int flags, int fd, off_t offset)
{
    int track = observing && (protection & PROT_WRITE) && (flags & MAP_ANONYMOUS);
    if (track)
    {
        struct timespec deadline;
        clock_gettime(CLOCK_REALTIME, &deadline);
        deadline.tv_nsec += 250000000;
        if (deadline.tv_nsec >= 1000000000)
        {
            deadline.tv_sec++;
            deadline.tv_nsec -= 1000000000;
        }
        pthread_mutex_lock(&ledger);
        arrived++;
        pthread_cond_broadcast(&changed);
        while (arrived < 8)
        {
            if (pthread_cond_timedwait(&changed, &ledger, &deadline))
            {
                break;
            }
        }
        pthread_mutex_unlock(&ledger);
    }
    void *result = __real_mmap64(address, size, protection, flags, fd, offset);
    if (track && result != MAP_FAILED)
    {
        pthread_mutex_lock(&ledger);
        if (allocated == 8)
        {
            fprintf(stderr, "FAIL more than eight shared native mappings\n");
            _Exit(12);
        }
        pages[allocated++] = result;
        pthread_mutex_unlock(&ledger);
    }
    return result;
}


/* EN: Observe actual native page retirement and preserve munmap's return status.
 * RU: Наблюдать освобождение настоящих страниц и сохранить статус munmap. */
int __wrap_munmap(void *address, size_t size)
{
    int result = __real_munmap(address, size);
    if (!result)
    {
        pthread_mutex_lock(&ledger);
        for (unsigned at = 0; at < allocated; at++)
        {
            if (pages[at] == address)
            {
                pages[at] = NULL;
                released++;
                break;
            }
        }
        pthread_mutex_unlock(&ledger);
    }
    return result;
}


/* EN: After real GC/resident shutdown require one published and retired mapping.
 * RU: После настоящего GC/join потребовать одну публикацию и снятие страниц. */
void __wrap_dvm_close_worker_stop(void)
{
    __real_dvm_close_worker_stop();
    fprintf(stderr, "JIT_PUBLICATION allocated=%u freed=%u calls=%u\n", allocated, released, calls);
    fprintf(stderr, "JIT_FAILURE rejected=%u retries=%u\n", rejected, explicit_retries);
    if (allocated != 1 + CONTROL || released != 1 + CONTROL || calls != 512 + explicit_retries || rejected != CONTROL)
    {
        fprintf(stderr, "FAIL shared native code publication/retirement\n");
        _Exit(12);
    }
}


/* EN: Inject one genuine publication refusal before RX, then forward all actual
 * mprotect calls. The VM must free partial code, unlock and retry from no cache.
 * RU: Один раз отказать публикации до RX, затем передавать mprotect без изменений.
 * VM должна снять частичный код, mutex и повторить из состояния без кэша. */
int __wrap_mprotect(void *address, size_t size, int protection)
{
    int refuse = 0;
    pthread_mutex_lock(&ledger);
    if (CONTROL && observing && !rejected && (protection & PROT_EXEC))
    {
        rejected++;
        explicit_retries += explicit_call != 0;
        refuse = 1;
    }
    pthread_mutex_unlock(&ledger);
    if (refuse)
    {
        errno = EACCES;
        return -1;
    }
    return __real_mprotect(address, size, protection);
}
