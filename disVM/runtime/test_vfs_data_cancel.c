/* Copyright (c) 2026 bhgv
 * EN: Real guest provider cancellation; native jobs own no Prog/guest pointers.
 * RU: Настоящая guest provider cancellation; native jobs без Prog/guest pointers.
 */
#include "vfs/boot.h"
#include "vfs/export.h"
#include "vfs/cancel.h"
#include "file_gate.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; /* Fixture native counters/gates. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Job admission/queued tail readiness. */
static unsigned started; /* Native data jobs actually executed. */
static unsigned observed; /* Watch records already consumed. */
static unsigned prefixcalls; /* A cancelled readn must never execute a second fragment. */
static unsigned released; /* Native callback states retired. */
static unsigned opened; /* Actual independent stream ownership. */
static unsigned closed; /* Actual consuming close count. */
static int queued; /* A tail reserved the currently active FD gate. */
static DVM_FileGate *head; /* Borrowed native gate through live source FD pin. */
static _Thread_local DVM_FileGate *current_gate; /* Current guest's native FD admission, fixture only. */
int __real_dvm_namespace_boot_initialize(const char *directory);
void __real_dvm_namespace_boot_shutdown(void);
void __real_dvm_runtime_gc_native_enter(int released);
int __real_dvm_file_gate_enter(DVM_FileGate *gate, DVM_FileTicket *ticket);


/* EN/RU: Explicit contracts remain enabled in every build mode.
 * Явные contracts включены в каждом build mode. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "VFS data cancel: %s\n", why);
        exit(2);
    }
}


/* EN/RU: One admitted job has independent callback storage through unsubscribe/flight retirement.
 * Один admitted job имеет independent callback storage до retirement callbacks. */
typedef struct Job
{
    pthread_mutex_t lock; /* Protects native stop predicate. */
    pthread_cond_t changed; /* Cancellation wakes only this native job. */
    int stopped; /* Sticky native signal, never a guest request poll. */
} Job;


/* EN/RU: Signal only native synchronization, with no provider/VM/host-ledger reentry.
 * Сигнализировать только native synchronization, без provider/VM/host-ledger reentry. */
static void interrupt(void *opaque)
{
    Job *job = opaque;
    pthread_mutex_lock(&job->lock);
    job->stopped = 1;
    pthread_cond_broadcast(&job->changed);
    pthread_mutex_unlock(&job->lock);
}


/* EN/RU: Consume callback state after all admitted interrupt signals finish.
 * Поглотить callback state после всех admitted interrupt signals. */
static void releasejob(void *opaque)
{
    Job *job = opaque;
    pthread_cond_destroy(&job->changed);
    pthread_mutex_destroy(&job->lock);
    free(job);
    pthread_mutex_lock(&lock);
    released++;
    pthread_mutex_unlock(&lock);
}


/* EN/RU: A stream copies a native name; wrapper owns its actual provider lifetime.
 * Stream копирует native name; wrapper владеет actual provider lifetime. */
typedef struct Stream
{
    char name[16]; /* Native fixture endpoint, not guest storage. */
} Stream;


/* EN/RU: Immutable byte endpoints include cancellable read/write/zero and native watch controls.
 * Immutable byte endpoints: cancellable read/write/zero и native watch controls. */
static FVM_Result info(void *opaque, const char *path, FVM_FileInfo *out)
{
    (void)opaque;
    if (strcmp(path, ".") && strcmp(path, "read") && strcmp(path, "readn") && strcmp(path, "write") && strcmp(path, "zero") &&
        strcmp(path, "watch") && strcmp(path, "queued") && strcmp(path, "fast"))
    {
        return FVM_NOT_FOUND;
    }
    memset(out, 0, sizeof(*out));
    out->directory = !strcmp(path, ".");
    out->mode = out->directory ? 0555 : 0666;
    out->identity.low = out->directory ? 1 : (unsigned char)*path;
    strcpy(out->name, out->directory ? "root" : path);
    return FVM_OK;
}


/* EN/RU: Wait for real native cancellation after publishing admission, preserving explicit cause.
 * Ждать real native cancellation после публикации admission; сохранить explicit cause. */
static void data(void *opaque, void *bytes, size_t size, DVM_IoResult *out)
{
    Stream *stream = opaque;
    memset(out, 0, sizeof(*out));
    out->flags = DVM_IO_PREFIX;
    if (!strcmp(stream->name, "fast"))
    {
        check(dvm_cancel_current() != NULL, "fresh operation lost native scope");
        out->status = FVM_OK;
        out->count = size ? 1 : 0;
        if (size)
        {
            *(char *)bytes = 'F';
        }
        return;
    }
    Job *job = calloc(1, sizeof(*job));
    check(job && !pthread_mutex_init(&job->lock, NULL) && !pthread_cond_init(&job->changed, NULL), "native job");
    DVM_CancelTap *tap = dvm_cancel_subscribe(dvm_cancel_current(), interrupt, job, releasejob);
    check(tap != NULL, "missing provider cancellation scope");
    pthread_mutex_lock(&lock);
    head = current_gate;
    if (!strcmp(stream->name, "readn"))
    {
        check(++prefixcalls == 1, "cancelled readn issued another provider chunk");
    }
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
    head = NULL;
    pthread_mutex_unlock(&lock);
    out->status = FVM_ERROR;
    out->stop = DVM_IO_CANCELLED;
    if (!strcmp(stream->name, "readn"))
    {
        check(size >= 1, "readn capacity");
        *(char *)bytes = 'A';
        out->status = FVM_OK;
        out->count = 1;
        out->stop = DVM_IO_NONE;
    }
    dvm_cancel_unsubscribe(tap);
}


/* EN/RU: Immutable write buffer is never modified by the controlled native job.
 * Immutable write buffer не меняется controlled native job. */
static void writechunk(void *opaque, const void *bytes, size_t size, DVM_IoResult *out)
{
    data(opaque, (void *)bytes, size, out);
}


/* EN/RU: Observe native admission/tail readiness on independent non-cancellable control FDs.
 * Наблюдать native admission/tail readiness через independent control FDs. */
static FVM_Result watch(void *opaque, void *bytes, size_t size, size_t *count)
{
    Stream *stream = opaque;
    pthread_mutex_lock(&lock);
    while (!strcmp(stream->name, "queued") ? !queued : observed == started)
    {
        pthread_cond_wait(&changed, &lock);
    }
    if (strcmp(stream->name, "queued"))
    {
        observed++;
    }
    pthread_mutex_unlock(&lock);
    *count = size ? 1 : 0;
    if (size)
    {
        *(char *)bytes = 'R';
    }
    return FVM_OK;
}


/* EN/RU: Consume exclusive native stream after all rooted FD borrowers retire.
 * Поглотить exclusive native stream после retirement всех rooted FD borrowers. */
static void closefile(void *opaque)
{
    free(opaque);
    pthread_mutex_lock(&lock);
    closed++;
    pthread_mutex_unlock(&lock);
}
static const DVM_VfsOps workops = {NULL, NULL, NULL, NULL, NULL, closefile, NULL, data, writechunk}; /* Native scope consumer. */
static const DVM_VfsOps watchops = {NULL, watch, NULL, NULL, NULL, closefile}; /* Fixture readiness control. */


/* EN/RU: Publish explicit per-handle cancellation capability only for data endpoints.
 * Публиковать explicit per-handle cancellation capability только для data endpoints. */
static FVM_Result openfile(void *opaque, const char *path, const char *mode, DVM_VfsHandle *out)
{
    FVM_FileInfo meta;
    (void)mode;
    if (info(opaque, path, &meta) != FVM_OK || meta.directory)
    {
        return FVM_NOT_FOUND;
    }
    Stream *stream = calloc(1, sizeof(*stream));
    check(stream != NULL, "native stream");
    strcpy(stream->name, path);
    int control = !strcmp(path, "watch") || !strcmp(path, "queued");
    out->ops = control ? &watchops : &workops;
    out->state = stream;
    out->native_flags = control ? 0 : DVM_VFS_CANCEL_DATA;
    pthread_mutex_lock(&lock);
    opened++;
    pthread_mutex_unlock(&lock);
    return FVM_OK;
}


/* EN/RU: Static fixture source needs no native allocation cleanup.
 * Static fixture source не требует allocation cleanup. */
static void destroy(void *opaque)
{
    (void)opaque;
}
static const DVM_ProviderOps ops = {info, openfile, destroy, DVM_PROVIDER_STABLE_BIND}; /* Immutable fixture namespace. */


/* EN/RU: Mount actual provider before guest startup, through normal namespace APIs.
 * Смонтировать actual provider до guest startup через normal namespace APIs. */
int __wrap_dvm_namespace_boot_initialize(const char *directory)
{
    if (!__real_dvm_namespace_boot_initialize(directory))
    {
        return 0;
    }
    DVM_Provider *provider = dvm_provider_create(&ops, NULL);
    DVM_NamespacePlan target;
    check(provider && dvm_namespace_plan(dvm_namespace_boot_group(), "remote", &target) &&
          dvm_namespace_mount_provider(dvm_namespace_boot_group(), &target, provider, "/interruptible-fixture") == 1, "mount source");
    dvm_provider_release(provider);
    return 1;
}


/* EN/RU: Observe actual native gate admission; a reserved tail remains cancellable independently.
 * Наблюдать actual native gate admission; reserved tail отменяется независимо. */
int __wrap_dvm_file_gate_enter(DVM_FileGate *gate, DVM_FileTicket *ticket)
{
    pthread_mutex_lock(&lock);
    if (head && head == gate)
    {
        queued = 1;
        pthread_cond_broadcast(&changed);
    }
    pthread_mutex_unlock(&lock);
    current_gate = gate;
    return __real_dvm_file_gate_enter(gate, ticket);
}


/* EN/RU: Every native reentry must see the provider scope already unlinked and cleared.
 * Каждый native reentry должен видеть provider scope уже unlinked/cleared. */
void __wrap_dvm_runtime_gc_native_enter(int state)
{
    check(dvm_cancel_current() == NULL, "cancel scope survived managed reentry");
    __real_dvm_runtime_gc_native_enter(state);
}


/* EN/RU: Real shutdown drains every FD and all callback state; no fixture teardown substitute.
 * Real shutdown освобождает все FD/callback state; без fixture teardown substitute. */
void __wrap_dvm_namespace_boot_shutdown(void)
{
    __real_dvm_namespace_boot_shutdown();
    check(started == 8 && released == started && opened == closed, "native jobs/FD ownership leak");
    puts("PASS native VFS cancel jobs and FD lifetimes drained");
}
