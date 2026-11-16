/* Copyright (c) 2026 bhgv
 * EN: Actual guest FD finalization, slot reuse and shutdown cancellation of detached native closes.
 * RU: Настоящая финализация guest FD, reuse slots и shutdown cancellation отделённых native close.
 */
#include "lib9.h"
#include "dat.h"
#include "interp.h"
#include "runtime_gc.h"
#include "close_worker.h"
#include "file_slots.h"
#include "vfs/boot.h"
#include "vfs/resource.h"
#include "vfs/cancel.h"
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static pthread_mutex_t observations = PTHREAD_MUTEX_INITIALIZER; /* Native observation predicates only. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Actual detached-job/close admission readiness. */
static uintptr_t identities[64]; /* Copied outer handle identities; never dereferenced. */
static int slots[64]; /* Copied descriptor numbers prove reuse before native close completion. */
static unsigned published; /* Actual provider slots published while armed. */
static unsigned submitted; /* Actual ownership transfers to the native FIFO. */
static unsigned closed; /* Exact consuming closes, including directories. */
static unsigned checkpoints; /* Producer retirement checkpoints. */
static unsigned files; /* Payload-checked native regular files. */
static unsigned directories; /* Real namespace directory wrappers retired. */
static int armed; /* Observation starts after loader cleanup. */
static int held; /* First close has subscribed to the independent source. */
static int stopping; /* Actual service stop began after every guest checkpoint. */
int __real_dvm_namespace_boot_initialize(const char *directory);
void __real_dvm_close_submit_resource(DVM_CloseJob *job, DVM_VfsHandle *resource);
void __real_dvm_file_slots_publish_resource(int descriptor, DVM_VfsHandle *resource);
void __real_dvm_close_worker_stop(void);
void __real_Sys_werrstr(void *frame);
Channel *__real_dvm_runtime_channel_new(void *, Type *, WORD, void (*)(void), int);


/* EN/RU: Verify real-guest ownership and cancellation contracts; no disabled assertions.
 * Проверить ownership/cancellation contracts настоящего guest во всех build modes. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "guest native close cancellation: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Each stream owns copied identity and payload state; no guest pointers.
 * Каждый stream владеет copied identity/payload state; без guest pointers. */
typedef struct Stream
{
    unsigned id; /* Copied producer0..63. */
    int directory; /* Native list adapter or regular file. */
    int written; /* Exact expected payload was acknowledged before retirement. */
} Stream;


/* EN/RU: Independent cancellation callback storage, not a provider/stream/guest object.
 * Независимое callback storage; не provider/stream/guest object. */
typedef struct Wake
{
    pthread_mutex_t lock; /* Protect idempotent native delivery. */
    pthread_cond_t changed; /* Native close interruption predicate. */
    int stopped; /* Sticky wake signal. */
} Wake;


/* EN/RU: Wake only native signal storage, never reenter service/VM/provider locks.
 * Будить только native signal storage, без service/VM/provider locks. */
static void interrupt(void *state)
{
    Wake *wake = state;
    pthread_mutex_lock(&wake->lock);
    wake->stopped = 1;
    pthread_cond_broadcast(&wake->changed);
    pthread_mutex_unlock(&wake->lock);
}


/* EN/RU: Consume signal storage after all admitted callback flights retire.
 * Поглотить signal storage после всех admitted callback flights. */
static void release_wake(void *state)
{
    Wake *wake = state;
    pthread_cond_destroy(&wake->changed);
    pthread_mutex_destroy(&wake->lock);
    free(wake);
}


/* EN/RU: Expose ordinary root/files/directories through the mounted native source.
 * Показать обычные root/files/directories через mounted native source. */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *out)
{
    (void)state;
    int directory = !strcmp(path, ".") || !strncmp(path, "dir", 3);
    if (strcmp(path, ".") && strncmp(path, "item", 4) && strncmp(path, "dir", 3))
    {
        return FVM_NOT_FOUND;
    }
    memset(out, 0, sizeof(*out));
    out->directory = directory;
    out->mode = directory ? 0555 : 0666;
    strcpy(out->name, directory ? "directory" : "item");
    return FVM_OK;
}


/* EN/RU: Acknowledge only exact producer bytes, before FD detachment.
 * Подтвердить только точные producer bytes до FD detachment. */
static FVM_Result writefile(void *state, const void *bytes, size_t size)
{
    Stream *stream = state;
    check(!stream->directory && size == 32, "write shape");
    for (size_t i = 0; i < size; i++)
    {
        check(((const unsigned char *)bytes)[i] == stream->id+1, "write payload");
    }
    stream->written = 1;
    return FVM_OK;
}


/* EN/RU: Return one real owned directory entry; ordinary guest dirread consumes it.
 * Вернуть одну настоящую directory entry; её получает ordinary guest dirread. */
static FVM_Result list(void *state, FVM_FileInfo **entries, size_t *count)
{
    check(((Stream *)state)->directory, "directory stream kind");
    *entries = calloc(1, sizeof(**entries));
    check(*entries != NULL, "directory entry allocation");
    strcpy((*entries)->name, "entry");
    (*entries)->mode = 0444;
    *count = 1;
    return FVM_OK;
}


/* EN/RU: Cursor publication abort touches native metadata only; no I/O or ownership change.
 * Cursor publication abort касается только native metadata, без I/O/ownership change. */
static void abortlist(void *state)
{
    (void)state;
}


/* EN/RU: Consume each detached stream once; only first waits for real shutdown signal.
 * Поглотить detached stream один раз; только первый ждёт настоящий shutdown signal. */
static void closefile(void *state)
{
    Stream *stream = state;
    DVM_Cancel *source = dvm_cancel_current();
    check(source != NULL, "detached close lacks native cancellation scope");
    if (!stream->id)
    {
        Wake *wake = calloc(1, sizeof(*wake));
        check(wake && !pthread_mutex_init(&wake->lock, NULL) && !pthread_cond_init(&wake->changed, NULL), "wake setup");
        DVM_CancelTap *tap = dvm_cancel_subscribe(source, interrupt, wake, release_wake);
        check(tap != NULL, "close source subscription");
        pthread_mutex_lock(&observations);
        held = 1;
        pthread_cond_broadcast(&changed);
        pthread_mutex_unlock(&observations);
        pthread_mutex_lock(&wake->lock);
        while (!wake->stopped)
        {
            pthread_cond_wait(&wake->changed, &wake->lock);
        }
        pthread_mutex_unlock(&wake->lock);
        dvm_cancel_unsubscribe(tap);
    }
    else
    {
        check(dvm_cancel_requested(source), "queued close did not observe shutdown cancellation");
    }
    pthread_mutex_lock(&observations);
    check(stopping && stream->id == closed && (stream->directory || stream->written), "ownership order or payload");
    closed++;
    files += !stream->directory;
    directories += stream->directory;
    pthread_mutex_unlock(&observations);
    free(stream);
}
static const DVM_VfsOps file_ops = {NULL, NULL, writefile, NULL, NULL, closefile}; /* Regular native stream. */
static const DVM_VfsOps dir_ops = {NULL, NULL, NULL, NULL, list, closefile, abortlist}; /* Namespace-projected native directory. */


/* EN/RU: Copy a bounded producer identity into a fresh exclusively owned native stream.
 * Копировать bounded producer identity в новый exclusive native stream. */
static FVM_Result openfile(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    FVM_FileInfo meta;
    (void)mode;
    if (!strcmp(path, ".") || info(state, path, &meta) != FVM_OK)
    {
        return FVM_ERROR;
    }
    Stream *stream = calloc(1, sizeof(*stream));
    check(stream != NULL, "stream allocation");
    stream->directory = meta.directory;
    stream->id = (unsigned)atoi(path+(meta.directory ? 3 : 4));
    check(stream->id < 64, "producer identity");
    out->state = stream;
    out->ops = stream->directory ? &dir_ops : &file_ops;
    out->native_flags = DVM_VFS_CANCEL_CLOSE;
    return FVM_OK;
}


/* EN/RU: Factory state is static; final drop requires no external/native I/O.
 * Factory state static; final drop не требует external/native I/O. */
static void destroy_factory(void *state)
{
    (void)state;
}
static const DVM_ProviderOps factory_ops = {info, openfile, destroy_factory, DVM_PROVIDER_STABLE_BIND}; /* Ordinary path/source contract. */


/* EN/RU: Install trusted provider before any guest owner starts.
 * Установить trusted provider до запуска guest owners. */
int __wrap_dvm_namespace_boot_initialize(const char *directory)
{
    if (!__real_dvm_namespace_boot_initialize(directory))
    {
        return 0;
    }
    DVM_Provider *source = dvm_provider_create(&factory_ops, NULL);
    DVM_NamespacePlan target;
    check(source && dvm_namespace_plan(dvm_namespace_boot_group(), "remote", &target) &&
          dvm_namespace_mount_provider(dvm_namespace_boot_group(), &target, source, "/native-close-fixture") == 1, "source mount");
    dvm_provider_release(source);
    return 1;
}


/* EN/RU: Record native publication identity before ownership moves; never inspect guest storage.
 * Записать native publication identity до ownership move, не читать guest storage. */
void __wrap_dvm_file_slots_publish_resource(int descriptor, DVM_VfsHandle *resource)
{
    pthread_mutex_lock(&observations);
    if (armed)
    {
        check(published < 64, "publication count");
        identities[published] = (uintptr_t)resource->state;
        slots[published++] = descriptor;
    }
    pthread_mutex_unlock(&observations);
    __real_dvm_file_slots_publish_resource(descriptor, resource);
}


/* EN/RU: Observe actual preallocated FIFO transfer; no callbacks/I/O inside finalization.
 * Наблюдать настоящий FIFO transfer; без callbacks/I/O в finalization. */
void __wrap_dvm_close_submit_resource(DVM_CloseJob *job, DVM_VfsHandle *resource)
{
    uintptr_t identity = (uintptr_t)resource->state;
    __real_dvm_close_submit_resource(job, resource);
    pthread_mutex_lock(&observations);
    if (armed && submitted < published && identities[submitted] == identity)
    {
        submitted++;
        pthread_cond_broadcast(&changed);
    }
    pthread_mutex_unlock(&observations);
}


/* EN/RU: Publish real parent roots and await this producer's actual native retirement job.
 * Опубликовать parent roots и ждать настоящую native retirement job производителя. */
void __wrap_Sys_werrstr(void *frame)
{
    __real_Sys_werrstr(frame);
    int native = dvm_runtime_gc_native_try_leave();
    check(native > 0, "checkpoint heap release");
    pthread_mutex_lock(&observations);
    unsigned expected = ++checkpoints;
    while (submitted < expected || (expected == 64 && !held))
    {
        pthread_cond_wait(&changed, &observations);
    }
    if (expected == 64)
    {
        check(published == 64 && submitted == 64 && !closed, "native backlog before shutdown");
    }
    pthread_mutex_unlock(&observations);
    dvm_runtime_gc_native_enter(native);
}


/* EN/RU: Arm after loader cleanup, preserving the actual managed channel constructor.
 * Arm после loader cleanup, сохранив настоящий managed channel constructor. */
Channel *__wrap_dvm_runtime_channel_new(void *destination, Type *type, WORD capacity, void (*mover)(void), int guarded)
{
    if (capacity == 105)
    {
        pthread_mutex_lock(&observations);
        armed = 1;
        pthread_mutex_unlock(&observations);
    }
    return __real_dvm_runtime_channel_new(destination, type, capacity, mover, guarded);
}


/* EN/RU: Positive eligibility is unchanged; private control suppresses native scope admission.
 * Positive eligibility без изменений; private control убирает native scope admission. */
int __real_dvm_vfs_close_cancellable(const DVM_VfsHandle *resource);
int __wrap_dvm_vfs_close_cancellable(const DVM_VfsHandle *resource)
{
#if CONTROL
    (void)resource;
    return 0;
#else
    return __real_dvm_vfs_close_cancellable(resource);
#endif
}


/* EN/RU: Stop only after managed producers retire, then verify FIFO drain and descriptor reuse.
 * Остановить после managed producers, затем проверить FIFO drain и reuse descriptors. */
void __wrap_dvm_close_worker_stop(void)
{
    pthread_mutex_lock(&observations);
    check(checkpoints == 64 && !closed && held, "shutdown must follow complete detached backlog");
    stopping = 1;
    pthread_mutex_unlock(&observations);
    __real_dvm_close_worker_stop();
    unsigned reused = 0;
    for (unsigned i = 1; i < 64; i++)
    {
        for (unsigned j = 0; j < i; j++)
        {
            if (slots[i] == slots[j])
            {
                reused++;
                break;
            }
        }
    }
    check(closed == 64 && files == 32 && directories == 32 && reused >= 35 && !dvm_cancel_current(), "drain or slot reuse");
    puts("PASS native guest close cancellation files=32 directories=32 consumed=64");
}
