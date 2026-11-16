/* Copyright (c) 2026 bhgv
 * EN: Actual guest layer inspection and move cancellation with native-only callback ownership.
 * RU: Отмена гостевых просмотра слоёв и move с независимым native callback state.
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
static unsigned acknowledgements; /* Committed move results preserved after cancellation. */
static unsigned captures; /* Captures in the current explicitly armed native operation. */
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
        fprintf(stderr, "VFS inspection wait: %s\n", why);
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
    check(tap != NULL, "inspection method lacks native cancellation scope");
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


/* EN/RU: Native factory identity is copied into fresh captured child roots.
 * Native factory identity копируется в fresh captured child roots. */
typedef struct Node
{
    int identity; /* Original/captured A0 or B1, retained by native factory ownership. */
} Node;


/* EN/RU: Return live metadata; only selected inspection waits, never namespace locks.
 * Вернуть live metadata; ждать только selected inspection, не удерживая namespace locks. */
static FVM_Result info(void *opaque, const char *path, FVM_FileInfo *out)
{
    (void)opaque;
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        unexpected++;
    }
    if (!strcmp(path, "meta"))
    {
        held();
        return FVM_ERROR;
    }
    memset(out, 0, sizeof(*out));
    out->directory = !strcmp(path, ".") || !strcmp(path, "sub");
    out->mode = out->directory ? 0555 : 0666;
    strcpy(out->name, out->directory ? "directory" : "item");
    return FVM_OK;
}


/* EN/RU: Observe actual permission callbacks and block only the chosen bit.
 * Cancellation must prevent a later bit; results remain diagnostic, not authority.
 * Наблюдать настоящие permission callbacks; ждать выбранный bit, отмена запрещает следующий. */
static FVM_Result access_check(void *opaque, const char *path, unsigned access)
{
    (void)opaque;
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        unexpected++;
    }
    if ((!strcmp(path, "rights") && access == FVM_ACCESS_EXEC) ||
        (!strcmp(path, "second") && access == FVM_ACCESS_WRITE) ||
        (!strcmp(path, "ack") && access == FVM_ACCESS_READ))
    {
        held();
        return !strcmp(path, "ack") ? FVM_OK : FVM_ERROR;
    }
    return access == FVM_ACCESS_WRITE ? FVM_ERROR : FVM_OK;
}


/* EN/RU: Native stream borrows its factory identity and owns copied endpoint text.
 * Native stream заимствует identity factory и владеет copied endpoint text. */
typedef struct Stream
{
    Node *node; /* Borrowed through outer factory lifetime pin. */
    char name[32]; /* Independent copied endpoint. */
} Stream;


/* EN/RU: Watch only actual native admission; ordinary reads verify destination identity.
 * Watch наблюдает native admission; ordinary reads проверяют destination identity. */
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
        *(char *)bytes = !strcmp(stream->name, "watch") ? 'W' : stream->node->identity ? 'B' : 'A';
    }
    return FVM_OK;
}


/* EN/RU: Select a copied move boundary after all inspection/setup work.
 * Выбрать copied move boundary после inspection/setup. */
static FVM_Result writefile(void *state, const void *bytes, size_t size)
{
    (void)state;
    check(size == 1, "arm shape");
    pthread_mutex_lock(&lock);
    armed = *(const unsigned char *)bytes;
    captures = 0;
    pthread_mutex_unlock(&lock);
    return FVM_OK;
}


/* EN/RU: Consume native stream after all FD borrowers retire.
 * Поглотить native stream после ухода FD borrowers. */
static void closefile(void *state)
{
    free(state);
    pthread_mutex_lock(&lock);
    closed++;
    pthread_mutex_unlock(&lock);
}
static const DVM_VfsOps stream_ops = {NULL, readfile, writefile, NULL, NULL, closefile}; /* Native control and byte oracle. */


/* EN/RU: Open files only; the fixture never enumerates or opens actual directories.
 * Открыть только files; fixture не открывает и не перечисляет directories. */
static FVM_Result openfile(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    (void)mode;
    FVM_FileInfo metadata;
    if (info(state, path, &metadata) != FVM_OK || metadata.directory)
    {
        return FVM_ERROR;
    }
    Stream *stream = calloc(1, sizeof(*stream));
    check(stream && strlen(path) < sizeof(stream->name), "stream setup");
    stream->node = state;
    strcpy(stream->name, path);
    out->ops = &stream_ops;
    out->state = stream;
    pthread_mutex_lock(&lock);
    opened++;
    pthread_mutex_unlock(&lock);
    return FVM_OK;
}


/* EN/RU: Consume native factory identity after root/origin/stream references retire.
 * Поглотить factory identity после root/origin/stream references. */
static void destroy(void *state)
{
    free(state);
    pthread_mutex_lock(&lock);
    destroyed++;
    pthread_mutex_unlock(&lock);
}
static const DVM_ProviderOps operations = {info, openfile, destroy, 0, access_check}; /* Fresh descendants require capture. */


/* EN/RU: Acquire one root; interruption can refuse first capture or acknowledge the final root.
 * Захватить root; interruption может отказать first capture либо подтвердить final root. */
static DVM_Provider *capture(void *state, const char *path)
{
    (void)path;
    pthread_mutex_lock(&lock);
    int mode = armed;
    unsigned ordinal = ++captures;
    pthread_mutex_unlock(&lock);
    if (mode == 'M' || (mode == 'V' && ordinal == 2))
    {
        held();
        if (mode == 'M')
        {
            return NULL;
        }
    }
    Node *node = malloc(sizeof(*node));
    check(node != NULL, "captured identity");
    *node = *(Node *)state;
    DVM_Provider *root = dvm_provider_create_native(&operations, node, capture, 1,
        DVM_PROVIDER_CANCEL_PATH | DVM_PROVIDER_CANCEL_CAPTURE);
    check(root != NULL, "captured factory");
    pthread_mutex_lock(&lock);
    created++;
    pthread_mutex_unlock(&lock);
    return root;
}


/* EN/RU: Install two original identities before guest publication.
 * Установить две original identities до guest publication. */
int __wrap_dvm_namespace_boot_initialize(const char *directory)
{
    if (!__real_dvm_namespace_boot_initialize(directory))
    {
        return 0;
    }
    const char *names[] = {"remote", "lower"};
    for (unsigned i = 0; i < 2; i++)
    {
        Node *node = malloc(sizeof(*node));
        check(node != NULL, "original identity");
        node->identity = i;
        DVM_Provider *root = dvm_provider_create_native(&operations, node, capture, 1,
            DVM_PROVIDER_CANCEL_PATH | DVM_PROVIDER_CANCEL_CAPTURE);
        DVM_NamespacePlan target;
        check(root && dvm_namespace_plan(dvm_namespace_boot_group(), names[i], &target) &&
              dvm_namespace_mount_provider(dvm_namespace_boot_group(), &target, root, "/inspection-fixture") == 1, "mount fixture");
        created++;
        dvm_provider_release(root);
    }
    return 1;
}


/* EN/RU: No provider scope may survive managed heap reentry or result publication.
 * Provider scope не должен пережить managed heap reentry/result publication. */
void __wrap_dvm_runtime_gc_native_enter(int state)
{
    check(!dvm_cancel_current(), "inspection/move scope survived managed reentry");
    __real_dvm_runtime_gc_native_enter(state);
}


/* EN/RU: Hold only an actual successful move so cancellation must preserve its result.
 * Удержать только actual successful move; отмена должна сохранить его результат. */
int __real_dvm_stack_move(DVM_ResourcePlan *, uint64_t, uint64_t, int64_t);
int __wrap_dvm_stack_move(DVM_ResourcePlan *target, uint64_t revision, uint64_t layer, int64_t index)
{
    int result = __real_dvm_stack_move(target, revision, layer, index);
    pthread_mutex_lock(&lock);
    int active = armed == 'K';
    pthread_mutex_unlock(&lock);
    if (result == 1 && active && dvm_cancel_current())
    {
        held();
        acknowledgements++;
    }
    return result;
}


/* EN/RU: Negative control removes source admission only; real operations remain unchanged.
 * Negative control убирает только source admission; real operations без изменений. */
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


/* EN/RU: Require complete native job/FD/factory drain and no callback after cancellation.
 * Требовать полного native job/FD/factory drain и отсутствия callbacks после отмены. */
void __wrap_dvm_namespace_boot_shutdown(void)
{
    __real_dvm_namespace_boot_shutdown();
    check(started == 13 && released == started && opened == closed && created == destroyed &&
          acknowledgements == 1 && !unexpected, "inspection/move lifetime or callback continuation");
    puts("PASS native VFS inspection and move lifetimes and commit result");
}
