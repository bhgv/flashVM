/* Copyright (c) 2026 bhgv
 * EN: Actual guest cwd cancellation and commit races with native-only callback ownership.
 * RU: Отмена гостевого cwd и гонки commit с независимым native callback state.
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
static unsigned acknowledgements; /* Committed cwd results preserved after cancellation. */
static unsigned unexpected; /* Forbidden provider callbacks after cancellation. */
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
        fprintf(stderr, "VFS cwd wait: %s\n", why);
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
    check(tap != NULL, "cwd method lacks native cancellation scope");
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


/* EN/RU: Factory node owns only immutable native root kind.
 * Factory node владеет immutable native root kind, без guest pointers. */
typedef struct Node
{
    int captured; /* Fresh dot identity instead of original source routes. */
} Node;


/* EN/RU: Return live directory metadata, optionally waiting at original/fresh root boundary.
 * Вернуть metadata каталога; optional ожидание на original/fresh root boundary. */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *output)
{
    Node *node = state;
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        unexpected++;
    }
    if (!strcmp(path, "meta") || (node->captured && !strcmp(path, ".") && armed == 'I'))
    {
        held();
    }
    memset(output, 0, sizeof(*output));
    output->directory = strcmp(path, "arm") && strcmp(path, "watch");
    output->mode = output->directory ? 0555 : 0666;
    strcpy(output->name, output->directory ? "directory" : "control");
    return FVM_OK;
}


/* EN/RU: Wait on the real search query, acknowledging success after cancellation.
 * Ждать настоящий search query; подтвердить успех после отмены для проверки terminal отказа. */
static FVM_Result access_check(void *state, const char *path, unsigned access)
{
    (void)state;
    (void)access;
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        unexpected++;
    }
    if (!strcmp(path, "rights"))
    {
        held();
    }
    return FVM_OK;
}


/* EN/RU: Arm a copied scalar mode before the actual child cwd call.
 * Установить copied scalar mode перед actual child cwd. */
static FVM_Result writefile(void *state, const void *bytes, size_t size)
{
    (void)state;
    check(size == 1, "arm shape");
    armed = *(const unsigned char *)bytes;
    return FVM_OK;
}


/* EN/RU: Native admission observer waits for one real job, never guest polling.
 * Native observer ждёт одну настоящую admission, без guest polling. */
static FVM_Result readfile(void *state, void *bytes, size_t size, size_t *count)
{
    (void)state;
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
        *(char *)bytes = 'W';
    }
    return FVM_OK;
}


/* EN/RU: Consume stateless stream and count final close.
 * Поглотить stateless stream и посчитать final close. */
static void closefile(void *state)
{
    (void)state;
    pthread_mutex_lock(&lock);
    closed++;
    pthread_mutex_unlock(&lock);
}
static const DVM_VfsOps stream_ops = {NULL, readfile, writefile, NULL, NULL, closefile}; /* Native fixture observer/control. */


/* EN/RU: Transfer a stateless observer/control stream, no directory FD in this fixture.
 * Передать stateless observer/control stream, не directory FD. */
static FVM_Result openfile(void *state, const char *path, const char *mode, DVM_VfsHandle *output)
{
    (void)mode;
    if (strcmp(path, "arm") && strcmp(path, "watch"))
    {
        return FVM_ERROR;
    }
    output->ops = &stream_ops;
    output->state = state;
    pthread_mutex_lock(&lock);
    opened++;
    pthread_mutex_unlock(&lock);
    return FVM_OK;
}


/* EN/RU: Consume original/fresh root only after all native borrowers retire.
 * Поглотить original/fresh root после завершения native borrowers. */
static void destroy(void *state)
{
    free(state);
    pthread_mutex_lock(&lock);
    destroyed++;
    pthread_mutex_unlock(&lock);
}
static const DVM_ProviderOps operations = {info, openfile, destroy, 0, access_check}; /* Fresh per-node capture. */


/* EN/RU: Acquire fresh root even if the held capture acknowledged after cancellation.
 * Захватить fresh root, включая позднее подтверждение после отмены. */
static DVM_Provider *capture(void *state, const char *path)
{
    (void)state;
    (void)path;
    check(!dvm_cancel_requested(dvm_cancel_current()), "capture entered after cancellation");
    if (armed == 'C')
    {
        held();
    }
    Node *node = calloc(1, sizeof(*node));
    check(node != NULL, "root allocation");
    node->captured = 1;
    DVM_Provider *root = dvm_provider_create_native(&operations, node, capture, 1,
        DVM_PROVIDER_CANCEL_PATH | DVM_PROVIDER_CANCEL_CAPTURE);
    check(root != NULL, "fresh root");
    pthread_mutex_lock(&lock);
    created++;
    pthread_mutex_unlock(&lock);
    return root;
}


/* EN/RU: Install one native provider before permanent guest threads start.
 * Установить provider до запуска permanent guest threads. */
int __wrap_dvm_namespace_boot_initialize(const char *directory)
{
    if (!__real_dvm_namespace_boot_initialize(directory))
    {
        return 0;
    }
    Node *node = calloc(1, sizeof(*node));
    check(node != NULL, "source allocation");
    DVM_Provider *root = dvm_provider_create_native(&operations, node, capture, 1,
        DVM_PROVIDER_CANCEL_PATH | DVM_PROVIDER_CANCEL_CAPTURE);
    DVM_NamespacePlan target;
    check(root && dvm_namespace_plan(dvm_namespace_boot_group(), "remote", &target) &&
          dvm_namespace_mount_provider(dvm_namespace_boot_group(), &target, root, "/cwd-fixture") == 1,
          "source mount");
    pthread_mutex_lock(&lock);
    created++;
    pthread_mutex_unlock(&lock);
    dvm_provider_release(root);
    return 1;
}


/* EN/RU: Verify that native source lifetime ends before managed reentry.
 * Проверить завершение native source до managed reentry. */
void __wrap_dvm_runtime_gc_native_enter(int state)
{
    check(!dvm_cancel_current(), "cwd scope survived managed reentry");
    __real_dvm_runtime_gc_native_enter(state);
}


/* EN/RU: Hold an actual cwd commit to require success when cancellation arrives afterwards.
 * Удержать actual cwd commit; поздняя отмена должна сохранить success. */
int __real_dvm_namespace_chdir_resource(DVM_Namespace *, DVM_ResourcePlan *);
int __wrap_dvm_namespace_chdir_resource(DVM_Namespace *group, DVM_ResourcePlan *plan)
{
    int result = __real_dvm_namespace_chdir_resource(group, plan);
    if (result == 1 && armed == 'K')
    {
        held();
        acknowledgements++;
    }
    return result;
}


/* EN/RU: Negative control removes scope registration only, preserving real provider calls.
 * Negative control удаляет только scope registration; provider calls остаются настоящими. */
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


/* EN/RU: Require exact admitted jobs/stream/root retirement and one committed acknowledgement.
 * Требовать exact job/stream/root retirement и одно committed acknowledgement. */
void __wrap_dvm_namespace_boot_shutdown(void)
{
    __real_dvm_namespace_boot_shutdown();
    check(started == 9 && released == started && opened == closed && created == destroyed &&
          acknowledgements == 1 && !unexpected, "cwd operation ownership or post-cancel callback");
    puts("PASS native provider cwd cancellation lifetimes and commit");
}
