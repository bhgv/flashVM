/* Copyright (c) 2026 bhgv
 * EN: Actual guest dirread cancellation and protected batch publication with native-only callback ownership.
 * RU: Actual guest dirread cancellation and protected batch publication с native-only callback ownership.
 */
#include "vfs/boot.h"
#include "vfs/export.h"
#include "vfs/cancel.h"
#include "file_gate.h"
#include "vfs/native_wait.h"
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
static unsigned legacy_calls; /* Forbidden callbacks after a late legacy bind. */
static int legacy; /* Native identity for the legacy factory. */
static unsigned published; /* Actual protected managed batch publications. */
static _Thread_local int batch_pending; /* Source-thread native batch awaiting managed result publication. */
static _Thread_local int batch_reentered; /* Source scope retired and managed heap reentered. */
static int armed; /* Enable held-fid operation only after guest open/setup finishes. */
int __real_dvm_namespace_boot_initialize(const char *directory);
void __real_dvm_namespace_boot_shutdown(void);
void __real_dvm_runtime_gc_native_enter(int released);
FVM_Result __real_dvm_vfs_list(DVM_VfsHandle *handle, FVM_FileInfo **entries, size_t *count);
int __real_dvm_file_gate_leave(DVM_FileGate *gate, DVM_FileTicket *ticket);
int __real_kprotect(int (*callback)(void *), void *state);
int __real_dvm_vfs_gate_enter(DVM_FileGate *gate, DVM_FileTicket *ticket, DVM_VfsWait **wait);


/* EN/RU: Explicit contracts stay enabled in all optimized/runtime modes.
 * Явные contracts включены во всех optimized/runtime modes. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "VFS directory wait: %s\n", why);
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
    check(tap != NULL, "directory method lacks native cancellation scope");
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


/* EN/RU: Query current fixture metadata; the second prepared-view child can block.
 * Borrow native state/path/output, preserve output on cancellation failure.
 * Запросить current metadata; второй child view может ждать отмену, output при отказе сохранён. */
static FVM_Result info(void *opaque, const char *path, FVM_FileInfo *out)
{
    pthread_mutex_lock(&lock);
    if (opaque && armed)
    {
        legacy_calls++;
    }
    int active = armed;
    pthread_mutex_unlock(&lock);
    if (active && !strcmp(path, "prefix/second"))
    {
        held();
        return FVM_ERROR;
    }
    memset(out, 0, sizeof(*out));
    out->directory = !strcmp(path, ".") || !strcmp(path, "dir") ||
        !strcmp(path, "ack") || !strcmp(path, "prefix") || !strcmp(path, "empty") || !strcmp(path, "empty/graft");
    out->mode = out->directory ? 0555 : 0444;
    const char *name = strrchr(path, '/');
    strcpy(out->name, name ? name+1 : path);
    return FVM_OK;
}


/* EN/RU: Exclusive native stream, immutable copied endpoint spelling.
 * Exclusive native stream, immutable copied endpoint spelling. */
typedef struct Stream
{
    char name[32]; /* Copied fixture endpoint, no VM storage. */
} Stream;


/* EN/RU: Arm only after setup; independent control stream never blocks on guest memory.
 * Arm только после setup; independent control stream не ждёт guest memory. */
static FVM_Result arm(void *opaque, const void *bytes, size_t size)
{
    (void)bytes;
    (void)size;
    Stream *stream = opaque;
    pthread_mutex_lock(&lock);
    armed = strcmp(stream->name, "disarm") != 0;
    pthread_mutex_unlock(&lock);
    return FVM_OK;
}


/* EN/RU: Consume one actual native admission record on an independent control FD.
 * Прочитать actual native admission record через independent control FD. */
static FVM_Result watch(void *opaque, void *bytes, size_t size, size_t *count)
{
    (void)opaque;
    pthread_mutex_lock(&lock);
    while (observed == started)
    {
        pthread_cond_wait(&changed, &lock);
    }
    observed++;
    pthread_mutex_unlock(&lock);
    *count = size ? 1 : 0;
    if (size)
    {
        *(char *)bytes = 'R';
    }
    return FVM_OK;
}


/* EN/RU: Held metadata is nonblocking and remains tied to the copied endpoint.
 * Held metadata не блокируется и относится к copied endpoint; output borrowed. */
static FVM_Result streaminfo(void *opaque, FVM_FileInfo *out)
{
    return info(NULL, ((Stream *)opaque)->name, out);
}


/* EN/RU: One native directory wait or acknowledged/prefix batch under current scope.
 * Return owned complete entries; later reads are invalidated by production cancellation.
 * Один native directory wait либо acknowledged/prefix batch под current scope;
 * вернуть свой batch; последующие чтения инвалидирует production cancellation. */
static FVM_Result list(void *opaque, FVM_FileInfo **entries, size_t *count)
{
    Stream *stream = opaque;
    *entries = NULL;
    *count = 0;
    if (!strcmp(stream->name, "empty"))
    {
        return FVM_OK;
    }
    if (strcmp(stream->name, "prefix"))
    {
        held();
        if (strcmp(stream->name, "ack"))
        {
            return FVM_ERROR;
        }
    }
    size_t size = !strcmp(stream->name, "prefix") ? 2 : 1;
    FVM_FileInfo *batch = calloc(size, sizeof(*batch));
    check(batch != NULL, "batch allocation");
    for (size_t i = 0; i < size; i++)
    {
        strcpy(batch[i].name, i ? "second" : "first");
        batch[i].mode = 0444;
    }
    *entries = batch;
    *count = size;
    return FVM_OK;
}


/* EN/RU: Native cursor invalidation is local metadata only.
 * Native cursor invalidation — только local metadata. */
static void abortlist(void *opaque)
{
    (void)opaque;
}


/* EN/RU: Consume one independent native stream after its rooted FD borrowers retire.
 * Поглотить independent native stream после rooted FD borrowers. */
static void closefile(void *opaque)
{
    free(opaque);
    pthread_mutex_lock(&lock);
    closed++;
    pthread_mutex_unlock(&lock);
}
static const DVM_VfsOps streamops = {streaminfo, watch, arm, NULL, list, closefile, abortlist}; /* Native control fixture. */


/* EN/RU: Actual open can refuse or acknowledge a created stream after cancellation.
 * Actual open может отказать или подтвердить созданный stream после cancellation. */
static FVM_Result openfile(void *opaque, const char *path, const char *mode, DVM_VfsHandle *out)
{
    (void)mode;
    pthread_mutex_lock(&lock);
    if (opaque && armed)
    {
        legacy_calls++;
    }
    pthread_mutex_unlock(&lock);
    Stream *stream = calloc(1, sizeof(*stream));
    check(stream && strlen(path) < sizeof(stream->name), "stream setup");
    strcpy(stream->name, path);
    out->ops = &streamops;
    out->state = stream;
    FVM_FileInfo meta;
    info(NULL, path, &meta);
    out->native_flags = DVM_VFS_CANCEL_INFO | (meta.directory ? DVM_VFS_CANCEL_LIST : 0);
    pthread_mutex_lock(&lock);
    opened++;
    pthread_mutex_unlock(&lock);
    return FVM_OK;
}


/* EN/RU: Static fixture state remains owned until real runtime shutdown.
 * Static fixture state принадлежит проверке до real runtime shutdown. */
static void destroy(void *opaque)
{
    (void)opaque;
}
static const DVM_ProviderOps ops = {info, openfile, destroy, DVM_PROVIDER_STABLE_BIND}; /* Stable native paths and explicit control promise. */


/* EN/RU: Install primary/lower opt-in factories through the actual startup namespace API.
 * Установить primary/lower opt-in factories через actual startup namespace API. */
int __wrap_dvm_namespace_boot_initialize(const char *directory)
{
    if (!__real_dvm_namespace_boot_initialize(directory))
    {
        return 0;
    }
    DVM_Provider *provider = dvm_provider_create_native(&ops, NULL, NULL, 0,
        DVM_PROVIDER_CANCEL_PATH | DVM_PROVIDER_CANCEL_LIST);
    DVM_NamespacePlan target;
    check(provider && dvm_namespace_plan(dvm_namespace_boot_group(), "remote", &target) &&
          dvm_namespace_mount_provider(dvm_namespace_boot_group(), &target, provider, "/dirread-fixture") == 1, "mount");
    dvm_provider_release(provider);
    provider = dvm_provider_create(&ops, &legacy);
    check(provider && dvm_namespace_plan(dvm_namespace_boot_group(), "legacy", &target) &&
          dvm_namespace_mount_provider(dvm_namespace_boot_group(), &target, provider, "/legacy-fixture") == 1, "legacy mount");
    dvm_provider_release(provider);
    return 1;
}


/* EN/RU: No scope can survive managed reentry, including path operations without an FD.
 * Scope не переживает managed reentry, включая path operations без FD. */
void __wrap_dvm_runtime_gc_native_enter(int state)
{
    check(dvm_cancel_current() == NULL, "control scope survived managed reentry");
    __real_dvm_runtime_gc_native_enter(state);
    if (batch_pending)
    {
        batch_reentered = 1;
    }
}


/* EN/RU: Positive dispatch is unchanged; private control omits only native scope registration.
 * Gate/ticket/wait borrowed; negative control must be caught by the native method.
 * Positive dispatch без изменений; private control убирает только scope registration;
 * gate/ticket/wait borrowed, native method должен обнаружить отсутствие scope. */
int __wrap_dvm_vfs_gate_enter(DVM_FileGate *gate, DVM_FileTicket *ticket, DVM_VfsWait **wait)
{
#if CONTROL
    *wait = NULL;
    return dvm_file_gate_try_release(gate, ticket);
#else
    return __real_dvm_vfs_gate_enter(gate, ticket, wait);
#endif
}


/* EN/RU: Observe ordinary native list dispatch without changing its result or ownership.
 * Наблюдать native list dispatch без изменения результата и ownership. */
FVM_Result __wrap_dvm_vfs_list(DVM_VfsHandle *handle, FVM_FileInfo **entries, size_t *count)
{
    FVM_Result result = __real_dvm_vfs_list(handle, entries, count);
    if (handle->native_flags & DVM_VFS_CANCEL_LIST)
    {
        batch_pending = 1;
        batch_reentered = 0;
    }
    return result;
}


/* EN/RU: The native FD ticket must survive until managed reentry/publication.
 * Native FD ticket должен жить до managed reentry/publication; gate/ticket borrowed. */
int __wrap_dvm_file_gate_leave(DVM_FileGate *gate, DVM_FileTicket *ticket)
{
    if (batch_pending)
    {
        check(batch_reentered && !dvm_cancel_current(), "directory FIFO released before managed publication");
        batch_pending = batch_reentered = 0;
    }
    return __real_dvm_file_gate_leave(gate, ticket);
}


/* EN/RU: Observe the actual protected publication callback on the source owner only.
 * Наблюдать настоящий protected publication callback только на source owner. */
int __wrap_kprotect(int (*callback)(void *), void *state)
{
    if (batch_pending)
    {
        check(batch_reentered && !dvm_cancel_current(), "batch published with native scope or without heap reentry");
        pthread_mutex_lock(&lock);
        published++;
        pthread_mutex_unlock(&lock);
    }
    return __real_kprotect(callback, state);
}


/* EN/RU: Check actual runtime drain, not synthetic fixture cleanup.
 * Проверить actual runtime drain, не synthetic fixture cleanup. */
void __wrap_dvm_namespace_boot_shutdown(void)
{
    __real_dvm_namespace_boot_shutdown();
    check(started == 8 && released == started && opened == closed && published == 2 && !legacy_calls, "directory/FD lifetime coverage");
    puts("PASS native VFS directory wait lifetimes");
}
