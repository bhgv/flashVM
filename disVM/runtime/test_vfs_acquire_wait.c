/* Copyright (c) 2026 bhgv
 * EN: Actual guest capture/bind acquisition cancellation with native-only callback ownership.
 * RU: Отмена гостевых capture/bind и владение независимым native callback state.
 */
#include "vfs/boot.h"
#include "vfs/export.h"
#include "vfs/cancel.h"
#include "vfs/native_wait.h"
#include "runtime_gc.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; /* Native fixture accounting. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Actual job admission notification. */
static unsigned started; /* Actual native control jobs, not speculative hints. */
static unsigned observed; /* Consumed admission records. */
static unsigned released; /* Exact callback-context retirement. */
static unsigned opened; /* Native stream constructors. */
static unsigned closed; /* Consuming stream close count. */
static unsigned created; /* Native node factories constructed. */
static unsigned destroyed; /* Native node factories retired. */
static unsigned acknowledgements; /* Committed bind/mount results preserved after cancellation. */
static int armed; /* Enable held-fid operation only after guest open/setup finishes. */
int __real_dvm_namespace_boot_initialize(const char *directory);
void __real_dvm_namespace_boot_shutdown(void);
void __real_dvm_runtime_gc_native_enter(int released);


/* EN/RU: Explicit contracts stay enabled in all optimized/runtime modes.
 * Явные contracts включены во всех optimized/runtime modes. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "VFS acquire wait: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Independent native callback state survives admitted signals after unsubscribe.
 * Independent native callback state переживает admitted signals после unsubscribe. */
typedef struct Job
{
    pthread_mutex_t lock; /* Protects sticky interruption. */
    pthread_cond_t changed; /* Native signal, never a guest continuation. */
    int stopped; /* Native cancellation delivered. */
} Job;


/* EN/RU: Signal native readiness without entering provider/VM/host ledger.
 * Сигнализировать native readiness без provider/VM/host ledger. */
static void interrupt(void *opaque)
{
    Job *job = opaque;
    pthread_mutex_lock(&job->lock);
    job->stopped = 1;
    pthread_cond_broadcast(&job->changed);
    pthread_mutex_unlock(&job->lock);
}


/* EN/RU: Consume callback state after its final admitted signal.
 * Поглотить callback state после final admitted signal. */
static void retire(void *opaque)
{
    Job *job = opaque;
    pthread_cond_destroy(&job->changed);
    pthread_mutex_destroy(&job->lock);
    free(job);
    pthread_mutex_lock(&lock);
    released++;
    pthread_mutex_unlock(&lock);
}


/* EN/RU: Publish actual control admission, then wait only on the current native cancel source.
 * Опубликовать actual control admission, ждать только current native cancel source. */
static void held(void)
{
    Job *job = calloc(1, sizeof(*job));
    check(job && !pthread_mutex_init(&job->lock, NULL) && !pthread_cond_init(&job->changed, NULL), "job setup");
    DVM_CancelTap *tap = dvm_cancel_subscribe(dvm_cancel_current(), interrupt, job, retire);
    check(tap != NULL, "acquire method lacks native cancellation scope");
    pthread_mutex_lock(&lock);
    started++;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&lock);
    pthread_mutex_lock(&job->lock);
    while (!job->stopped)
    {
        pthread_cond_wait(&job->changed, &job->lock);
    }
    pthread_mutex_unlock(&job->lock);
    pthread_mutex_lock(&lock);
    armed = 0;
    pthread_mutex_unlock(&lock);
    dvm_cancel_unsubscribe(tap);
}


/* EN/RU: Factory node owns only native scalar identity; streams borrow it under provider pins.
 * Factory node владеет только native scalar identity; streams заимствуют её под provider pins. */
typedef struct Node
{
    int captured; /* Fixed fresh root, distinct from original source. */
    int target; /* Original destination identity. */
} Node;


/* EN/RU: Current metadata; selected root queries wait on independent native wake state.
 * Output is copied only on success; validation can acknowledge after cancellation.
 * Current metadata; root queries ждут independent native wake; output только при success. */
static FVM_Result info(void *opaque, const char *path, FVM_FileInfo *out)
{
    Node *node = opaque;
    pthread_mutex_lock(&lock);
    int mode = armed;
    pthread_mutex_unlock(&lock);
    if (!strcmp(path, "metahold"))
    {
        held();
        return FVM_ERROR;
    }
    if ((!node->captured && !node->target && mode == 'R' && !strcmp(path, ".")) ||
        (node->target && mode == 'T' && !strcmp(path, ".")))
    {
        held();
        return FVM_ERROR;
    }
    if (((node->captured && mode == 'V') || (!node->target && !node->captured && mode == 'I')) && !strcmp(path, "."))
    {
        held();
    }
    memset(out, 0, sizeof(*out));
    out->directory = !strcmp(path, ".") || !strcmp(path, "hold") || !strcmp(path, "fast") || !strcmp(path, "ack");
    out->mode = out->directory ? 0555 : 0666;
    strcpy(out->name, !strcmp(path, ".") ? "root" : path);
    return FVM_OK;
}


/* EN/RU: Exclusive stream borrows factory node and copies endpoint spelling.
 * Exclusive stream заимствует factory node и копирует endpoint spelling. */
typedef struct Stream
{
    Node *node; /* Borrowed through outer provider lifetime pin. */
    char name[32]; /* Copied endpoint, never guest memory. */
} Stream;


/* EN/RU: Watch observes actual native admission; ordinary reads expose retained node identity.
 * Watch наблюдает actual native admission; обычное чтение даёт retained identity. */
static FVM_Result readfile(void *opaque, void *bytes, size_t size, size_t *count)
{
    Stream *stream = opaque;
    if (!strcmp(stream->name, "watch"))
    {
        pthread_mutex_lock(&lock);
        while (observed == started)
        {
            pthread_cond_wait(&changed, &lock);
        }
        observed++;
        pthread_mutex_unlock(&lock);
    }
    *count = size ? 1 : 0;
    if (size)
    {
        *(char *)bytes = !strcmp(stream->name, "watch") ? 'W' : stream->node->captured ? 'C' : stream->node->target ? 'T' : 'O';
    }
    return FVM_OK;
}


/* EN/RU: Arm one exact fixture boundary, without retaining incoming byte storage.
 * Включить одну точную fixture boundary без удержания входных bytes. */
static FVM_Result writefile(void *opaque, const void *bytes, size_t size)
{
    (void)opaque;
    if (size != 1)
    {
        return FVM_ERROR;
    }
    pthread_mutex_lock(&lock);
    armed = *(const unsigned char *)bytes;
    pthread_mutex_unlock(&lock);
    return FVM_OK;
}


/* EN/RU: Close independent stream after its borrowers retire; factory remains pinned outside.
 * Закрыть independent stream после borrowers; factory удерживается outer wrapper. */
static void closefile(void *opaque)
{
    free(opaque);
    pthread_mutex_lock(&lock);
    closed++;
    pthread_mutex_unlock(&lock);
}
static const DVM_VfsOps streamops = {NULL, readfile, writefile, NULL, NULL, closefile}; /* Ordinary native fixture stream. */


/* EN/RU: Open files only; test operations never require actual directory enumeration.
 * Открыть только files; directory enumeration не требуется для этих операций. */
static FVM_Result openfile(void *opaque, const char *path, const char *mode, DVM_VfsHandle *out)
{
    (void)mode;
    FVM_FileInfo meta;
    if (info(opaque, path, &meta) != FVM_OK || meta.directory)
    {
        return FVM_ERROR;
    }
    Stream *stream = calloc(1, sizeof(*stream));
    check(stream && strlen(path) < sizeof(stream->name), "stream setup");
    stream->node = opaque;
    strcpy(stream->name, path);
    out->ops = &streamops;
    out->state = stream;
    pthread_mutex_lock(&lock);
    opened++;
    pthread_mutex_unlock(&lock);
    return FVM_OK;
}


/* EN/RU: Consume one native factory node after every origin/handle reference retires.
 * Поглотить native factory node после всех origin/handle references. */
static void destroy(void *opaque)
{
    free(opaque);
    pthread_mutex_lock(&lock);
    destroyed++;
    pthread_mutex_unlock(&lock);
}
static const DVM_ProviderOps dynamic = {info, openfile, destroy, 0}; /* Fresh descendants need capture. */
static const DVM_ProviderOps stable = {info, openfile, destroy, DVM_PROVIDER_STABLE_BIND}; /* Destination has fixed relative routing. */


/* EN/RU: Return owned rooted node/NULL; cancellation can refuse or acknowledge native acquisition.
 * Вернуть свой rooted node/NULL; cancellation может отказать либо подтвердить native acquisition. */
static DVM_Provider *capture(void *opaque, const char *path)
{
    (void)opaque;
    if (!strcmp(path, "hold") || !strcmp(path, "ack"))
    {
        held();
        if (!strcmp(path, "hold"))
        {
            return NULL;
        }
    }
    Node *node = calloc(1, sizeof(*node));
    check(node != NULL, "captured node allocation");
    node->captured = 1;
    DVM_Provider *provider = dvm_provider_create_native(&dynamic, node, capture, 1,
        DVM_PROVIDER_CANCEL_PATH | DVM_PROVIDER_CANCEL_CAPTURE);
    check(provider != NULL, "captured node factory");
    pthread_mutex_lock(&lock);
    created++;
    pthread_mutex_unlock(&lock);
    return provider;
}


/* EN/RU: Install advanced source and stable destination before any guest starts.
 * Установить advanced source и stable destination до запуска guests. */
int __wrap_dvm_namespace_boot_initialize(const char *directory)
{
    if (!__real_dvm_namespace_boot_initialize(directory))
    {
        return 0;
    }
    const char *names[] = {"remote", "target"};
    for (unsigned i = 0; i < 2; i++)
    {
        Node *node = calloc(1, sizeof(*node));
        check(node != NULL, "source allocation");
        node->target = i != 0;
        DVM_Provider *provider = dvm_provider_create_native(i ? &stable : &dynamic, node, i ? NULL : capture, 1,
            DVM_PROVIDER_CANCEL_PATH | (i ? 0 : DVM_PROVIDER_CANCEL_CAPTURE));
        DVM_NamespacePlan target;
        check(provider && dvm_namespace_plan(dvm_namespace_boot_group(), names[i], &target) &&
              dvm_namespace_mount_provider(dvm_namespace_boot_group(), &target, provider, "/acquire-fixture") == 1, "mount");
        created++;
        dvm_provider_release(provider);
    }
    return 1;
}


/* EN/RU: Require source scope retirement before any managed result/reentry.
 * Требовать снятия source scope до managed result/reentry. */
void __wrap_dvm_runtime_gc_native_enter(int state)
{
    check(!dvm_cancel_current(), "acquisition scope survived managed reentry");
    __real_dvm_runtime_gc_native_enter(state);
}


/* EN/RU: Gate only a successful actual commit, preserving its original acknowledgement.
 * Удержать только успешный actual commit, сохранив исходный acknowledgement. */
int __real_dvm_stack_bind(DVM_ResourcePlan *, DVM_ResourcePlan *, unsigned, int64_t);
int __wrap_dvm_stack_bind(DVM_ResourcePlan *source, DVM_ResourcePlan *target, unsigned flags, int64_t index)
{
    int result = __real_dvm_stack_bind(source, target, flags, index);
    pthread_mutex_lock(&lock);
    int active = armed == 'A';
    pthread_mutex_unlock(&lock);
    if (result == 1 && active && dvm_cancel_current())
    {
        held();
        acknowledgements++;
    }
    return result;
}


/* EN/RU: Negative control drops source registration only; ordinary dispatch is unchanged.
 * Negative control убирает лишь source registration; ordinary dispatch без изменений. */
int __real_dvm_vfs_path_enter(DVM_VfsWait **wait);
int __wrap_dvm_vfs_path_enter(DVM_VfsWait **wait)
{
#if CONTROL
    *wait = NULL;
    return dvm_runtime_gc_native_try_leave();
#else
    return __real_dvm_vfs_path_enter(wait);
#endif
}


/* EN/RU: Verify exact native job/FD/factory drain after real runtime shutdown.
 * Проверить точный native job/FD/factory drain после настоящего runtime shutdown. */
void __wrap_dvm_namespace_boot_shutdown(void)
{
    __real_dvm_namespace_boot_shutdown();
    check(started == 17 && released == started && opened == closed && created == destroyed && acknowledgements == 3,
          "capture/bind lifetime or committed acknowledgement");
    puts("PASS native VFS acquire wait ownership and committed results");
}
