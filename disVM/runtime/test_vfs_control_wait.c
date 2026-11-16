/* Copyright (c) 2026 bhgv
 * EN: Actual guest path/open/fstat/seek cancellation with native-only callback ownership.
 * RU: Actual guest path/open/fstat/seek cancellation с native-only callback ownership.
 */
#include "vfs/boot.h"
#include "vfs/export.h"
#include "vfs/cancel.h"
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
static unsigned autocalls; /* Independent observation that MAUTORWX selected its actual native algorithm. */
static unsigned creation; /* Forbidden creation after a cancelled missing-entry lookup. */
static unsigned fallback; /* Forbidden lower-branch metadata calls after cancellation. */
static int armed; /* Enable held-fid operation only after guest open/setup finishes. */
static int secondary = 1; /* Distinguish independent lower native factory, no guest pointer. */
int __real_dvm_namespace_boot_initialize(const char *directory);
void __real_dvm_namespace_boot_shutdown(void);
void __real_dvm_runtime_gc_native_enter(int released);
FVM_Result __real_dvm_union_auto_select(DVM_ResourcePlan *plan, int creating);


/* EN/RU: Explicit contracts stay enabled in all optimized/runtime modes.
 * Явные contracts включены во всех optimized/runtime modes. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "VFS control wait: %s\n", why);
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
    check(tap != NULL, "control method lacks native cancellation scope");
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


/* EN/RU: Immutable roots/members; only explicit path/missing metadata waits for cancellation.
 * Immutable roots/members; только path/missing metadata ожидает cancellation. */
static FVM_Result info(void *opaque, const char *path, FVM_FileInfo *out)
{
    if (!strcmp(path, "statack"))
    {
        if (dvm_cancel_requested(dvm_cancel_current()))
        {
            return FVM_ERROR;
        }
        held();
    }
    if (!strcmp(path, "path") || !strcmp(path, "missing"))
    {
        if (opaque)
        {
            pthread_mutex_lock(&lock);
            fallback++;
            pthread_mutex_unlock(&lock);
        }
        else
        {
            held();
            return FVM_NOT_FOUND;
        }
    }
    memset(out, 0, sizeof(*out));
    out->directory = !strcmp(path, ".") || !strcmp(path, "dir");
    out->mode = out->directory ? 0555 : 0666;
    out->identity.low = out->directory ? 1 : (unsigned char)*path;
    out->length.low = 5;
    strcpy(out->name, !strcmp(path, ".") ? "root" : path);
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
    (void)opaque;
    (void)bytes;
    (void)size;
    pthread_mutex_lock(&lock);
    armed = 1;
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


/* EN/RU: Held-fid stat preserves output on cancellation; setup metadata remains nonblocking.
 * Held-fid stat сохраняет output при cancellation; setup metadata не блокируется. */
static FVM_Result streaminfo(void *opaque, FVM_FileInfo *out)
{
    Stream *stream = opaque;
    pthread_mutex_lock(&lock);
    int active = armed;
    pthread_mutex_unlock(&lock);
    if (active && (!strcmp(stream->name, "fstat") || !strcmp(stream->name, "dir")))
    {
        held();
        return FVM_ERROR;
    }
    return info(NULL, stream->name, out);
}


/* EN/RU: Model SEEK_END metadata wait; failed operation never modifies position output.
 * Смоделировать SEEK_END metadata wait; failure не меняет position output. */
static FVM_Result seek(void *opaque, long offset, int origin, long *position)
{
    (void)opaque;
    (void)offset;
    (void)origin;
    (void)position;
    held();
    return FVM_ERROR;
}


/* EN/RU: Directory type setup has a valid ordinary cursor; enumeration is outside this probe.
 * Directory setup имеет valid ordinary cursor; enumeration вне этой проверки. */
static FVM_Result list(void *opaque, FVM_FileInfo **entries, size_t *count)
{
    (void)opaque;
    *entries = NULL;
    *count = 0;
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
static const DVM_VfsOps streamops = {streaminfo, watch, arm, seek, list, closefile, abortlist}; /* Native control fixture. */


/* EN/RU: Actual open can refuse or acknowledge a created stream after cancellation.
 * Actual open может отказать или подтвердить созданный stream после cancellation. */
static FVM_Result openfile(void *opaque, const char *path, const char *mode, DVM_VfsHandle *out)
{
    (void)opaque;
    (void)mode;
    if (!strcmp(path, "missing"))
    {
        pthread_mutex_lock(&lock);
        creation++;
        pthread_mutex_unlock(&lock);
    }
    if (!strcmp(path, "open") || !strcmp(path, "openack"))
    {
        held();
        if (!strcmp(path, "open"))
        {
            return FVM_ERROR;
        }
    }
    Stream *stream = calloc(1, sizeof(*stream));
    check(stream && strlen(path) < sizeof(stream->name), "stream setup");
    strcpy(stream->name, path);
    out->ops = &streamops;
    out->state = stream;
    out->native_flags = !strcmp(path, "fstat") || !strcmp(path, "dir") ? DVM_VFS_CANCEL_INFO :
        !strcmp(path, "seek") ? DVM_VFS_CANCEL_SEEK : 0;
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
    const char *names[] = {"remote", "lower"};
    for (unsigned i = 0; i < 2; i++)
    {
        DVM_Provider *provider = dvm_provider_create_native(&ops, i ? &secondary : NULL, NULL, 0, DVM_PROVIDER_CANCEL_PATH);
        DVM_NamespacePlan target;
        check(provider && dvm_namespace_plan(dvm_namespace_boot_group(), names[i], &target) &&
              dvm_namespace_mount_provider(dvm_namespace_boot_group(), &target, provider, "/control-fixture") == 1, "mount");
        dvm_provider_release(provider);
    }
    DVM_Provider *creatable = dvm_provider_create_native(&ops, NULL, NULL, 0, DVM_PROVIDER_CANCEL_PATH);
    check(creatable && dvm_stack_mount(dvm_namespace_boot_group(), creatable, "create", DVM_NS_CREATE, 0) == 1,
          "create-capable native root");
    dvm_provider_release(creatable);
    return 1;
}


/* EN/RU: No scope can survive managed reentry, including path operations without an FD.
 * Scope не переживает managed reentry, включая path operations без FD. */
void __wrap_dvm_runtime_gc_native_enter(int state)
{
    check(dvm_cancel_current() == NULL, "control scope survived managed reentry");
    __real_dvm_runtime_gc_native_enter(state);
}


/* EN/RU: Observe actual automatic selection without replacing any native algorithm.
 * Наблюдать actual automatic selection, не подменяя native algorithm. */
FVM_Result __wrap_dvm_union_auto_select(DVM_ResourcePlan *plan, int creating)
{
    pthread_mutex_lock(&lock);
    autocalls++;
    pthread_mutex_unlock(&lock);
    return __real_dvm_union_auto_select(plan, creating);
}


/* EN/RU: Check actual runtime drain, not synthetic fixture cleanup.
 * Проверить actual runtime drain, не synthetic fixture cleanup. */
void __wrap_dvm_namespace_boot_shutdown(void)
{
    __real_dvm_namespace_boot_shutdown();
    check(!fallback, "cancelled union probed a lower provider");
    check(!creation, "cancelled lookup created a resource");
    check(started == 15 && released == started && opened == closed && autocalls, "control/FD lifetime or automatic-selection coverage");
    puts("PASS native VFS control wait lifetimes and no union fallback");
}
