/* Copyright (c) 2026 bhgv
 * EN: Inbox-wide atomic consumption, allocation-failure recovery and competing readers.
 * RU: Общий atomic consumption inbox, allocation-failure recovery и competing readers.
 */
#include "sysevents_internal.h"
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define QUEUE_READERS 2U /* Independent FDs consuming one native inbox. */
#define QUEUE_REPORTS 24U /* Fits without loss, so every report must arrive exactly once. */
static int fail_allocations; /* Test-main-only injection; zero during concurrent readers. */
static unsigned refused_allocations; /* Number of actual calloc attempts rejected by injection. */
void *__real_calloc(size_t count, size_t size); /* Production allocator, unchanged outside injection. */


/* EN/RU: Fail selected test-main allocations, preserving all ordinary calls.
 * При injection отказать calloc; остальные calls сохраняют production semantics. */
void *__wrap_calloc(size_t count, size_t size)
{
    if (fail_allocations)
    {
        refused_allocations++;
        return NULL;
    }
    return __real_calloc(count, size);
}


/* EN/RU: Fail explicitly with borrowed label; callable by native test readers.
 * Явный failure с borrowed label; допустим в native test readers. */
static void require(int ok, const char *label)
{
    if (!ok)
    {
        fprintf(stderr, "sysevents queue: %s\n", label);
        abort();
    }
}


/* EN/RU: Parsed experimental envelope used only as a test oracle, not frozen ABI.
 * Parsed experimental envelope — только test oracle, не frozen ABI. */
typedef struct Record
{
    unsigned kind; /* Logical system-event class, distinct from native message domain. */
    unsigned classes; /* Provider class mask. */
    uint64_t generation; /* Fixture sequence, zero for scalar control notices. */
} Record;


/* EN/RU: Read one whole record from borrowed FD, verify no batch/truncation.
 * Прочитать одну запись; проверить отсутствие batch/truncation. Return parsed oracle. */
static Record read_record(DVM_VfsHandle *fd)
{
    char bytes[DVM_SYSEVENT_FRAME_BYTES];
    uint64_t binding, session, epoch;
    Record record;
    DVM_IoResult result = dvm_vfs_read_chunk(fd, bytes, sizeof(bytes) - 1);
    require(result.status == FVM_OK && result.count && result.count < sizeof(bytes) &&
            (result.flags & DVM_IO_RECORD), "whole record outcome");
    bytes[result.count] = 0;
    require(strchr(bytes, '\n') == bytes + result.count - 1, "multiple or partial records");
    require(sscanf(bytes, "system %u %u %" SCNx64 " %" SCNx64 " %" SCNx64 " %" SCNx64,
            &record.kind, &record.classes, &binding, &session, &record.generation, &epoch) == 6,
            "malformed experimental envelope");
    return record;
}


/* EN/RU: Verify empty live/retired state without treating temporary refusal as EOF.
 * Проверить пустое live/retired состояние; временный отказ не EOF. */
static void empty(DVM_VfsHandle *fd, int retired)
{
    char bytes[DVM_SYSEVENT_FRAME_BYTES];
    DVM_IoResult result = dvm_vfs_read_chunk(fd, bytes, sizeof(bytes));
    require(!result.count && (retired ? result.status == FVM_OK && result.stop == DVM_IO_EOF :
            result.status != FVM_OK && result.stop == DVM_IO_AGAIN), "empty inbox status");
}


/* EN/RU: Publish a fixture sequence under the same native inbox lock as production.
 * Опубликовать fixture sequence под production inbox lock; caller controls allocation injection. */
static int post(DVM_SysEvents *events, uint64_t generation)
{
    int result;
    fvm_port_sync_lock(events->lock);
    result = dvm_sysevents_post(events, DVM_SYSEVENT_VFS, 7, DVM_CHANGE_CONTENT, 1, generation, "/visible");
    fvm_port_sync_unlock(events->lock);
    return result;
}


/* EN/RU: An undersized read must leave both destination and shared queue unchanged.
 * Short read не должен менять destination и shared queue, включая repeated reads. */
static void short_read(DVM_VfsHandle *fd)
{
    unsigned char bytes[5];
    DVM_IoResult result;
    memset(bytes, 0xa5, sizeof(bytes));
    result = dvm_vfs_read_chunk(fd, bytes, sizeof(bytes));
    require(result.status != FVM_OK && !result.count && !(result.flags & DVM_IO_RECORD), "undersized record accepted");
    for (size_t index = 0; index < sizeof(bytes); index++)
    {
        require(bytes[index] == 0xa5, "short read changed destination");
    }
}


/* EN/RU: Verify closing a short-reading FD cannot steal/reorder the inbox head.
 * Проверить, что close short-reading FD не крадёт/переставляет shared inbox head. */
static void shared_head(DVM_SysEvents *events, DVM_VfsHandle *first, DVM_VfsHandle *second)
{
    Record record;
    unsigned char sentinel = 0xa5;
    DVM_IoResult zero;
    require(post(events, 11) && post(events, 12), "shared-head publication");
    zero = dvm_vfs_read_chunk(first, &sentinel, 0);
    require(zero.status == FVM_OK && !zero.count && zero.stop == DVM_IO_NONE &&
            sentinel == 0xa5 && events->pending == 2, "zero read consumed a record or became EOF");
    short_read(first);
    short_read(first);
    require(events->pending == 2, "short read consumed shared head");
    dvm_vfs_close(first);
    record = read_record(second);
    require(record.kind == DVM_SYSEVENT_VFS && record.generation == 11, "FD close lost/reordered head");
    require(dvm_sysevents_open(events, "r", first) == FVM_OK, "FD reopen");
    record = read_record(first);
    require(record.generation == 12 && !events->pending, "reopened FD replayed or skipped record");
    empty(second, 0);
}


/* EN/RU: Overflow coalesces to one scalar resync; failed reads cannot clear it.
 * Overflow даёт один scalar resync; short reads его не сбрасывают. */
static void overflow(DVM_SysEvents *events, DVM_VfsHandle *first, DVM_VfsHandle *second)
{
    Record record;
    for (unsigned index = 1; index <= DVM_SYSEVENT_LIMIT + 1; index++)
    {
        require(post(events, index), "overflow publication");
    }
    require(events->resync && events->pending == 1, "overflow kept stale detail");
    short_read(first);
    dvm_vfs_close(first);
    record = read_record(second);
    require(record.kind == DVM_SYSEVENT_RESYNC && record.classes == DVM_CHANGE_ALL, "missing resync");
    record = read_record(second);
    require(record.kind == DVM_SYSEVENT_VFS && record.generation == DVM_SYSEVENT_LIMIT + 1,
            "duplicate resync or triggering report lost");
    empty(second, 0);
    require(dvm_sysevents_open(events, "r", first) == FVM_OK, "post-overflow reopen");
}


/* EN/RU: Queue allocations may fail repeatedly; preserve a single allocation-free loss notice.
 * Repeated allocation failures сохраняют один allocation-free loss notice. */
static void allocation_failure(DVM_SysEvents *events, DVM_VfsHandle *fd)
{
    Record record;
    require(post(events, 40), "pre-failure report");
    fail_allocations = 1;
    require(!post(events, 41) && !post(events, 42), "injection did not refuse real queue allocation");
    fail_allocations = 0;
    require(events->resync && refused_allocations == 2, "allocation loss became silent");
    short_read(fd);
    record = read_record(fd);
    require(record.kind == DVM_SYSEVENT_RESYNC, "allocation-free resync absent");
    record = read_record(fd);
    require(record.kind == DVM_SYSEVENT_VFS && record.generation == 40, "old confirmed report lost");
    empty(fd, 0);
}


/* EN/RU: Reader results share only their test counters; each FD stays thread-exclusive.
 * Results разделяют только test counters; каждый FD принадлежит одному native reader. */
typedef struct Readers
{
    pthread_mutex_t lock; /* Protects seen/count, never held during inbox operations. */
    unsigned seen[QUEUE_REPORTS]; /* Per-sequence consumption counts, all must equal one. */
    unsigned count; /* Total successful records across all independent reader FDs. */
} Readers;


/* EN/RU: Borrowed reader parameters valid through pthread_join.
 * Borrowed reader parameters живут до pthread_join. */
typedef struct Reader
{
    DVM_VfsHandle *fd; /* Independent FD, only this native thread accesses its outcome fields. */
    Readers *results; /* Shared test evidence, protected by its own mutex. */
} Reader;


/* EN/RU: Compete for queued records without a producer/target handshake or retries.
 * Читать queued records конкурентно без producer/target handshake/retries; stop on AGAIN. */
static void *reader(void *state)
{
    Reader *input = state;
    for (;;)
    {
        char bytes[DVM_SYSEVENT_FRAME_BYTES];
        uint64_t binding, session, generation;
        unsigned kind, classes;
        DVM_IoResult result = dvm_vfs_read_chunk(input->fd, bytes, sizeof(bytes) - 1);
        if (result.stop == DVM_IO_AGAIN)
        {
            require(!result.count, "empty reader acknowledged bytes");
            return NULL;
        }
        require(result.status == FVM_OK && result.count && result.count < sizeof(bytes) &&
                (result.flags & DVM_IO_RECORD), "concurrent record read");
        bytes[result.count] = 0;
        require(sscanf(bytes, "system %u %u %" SCNx64 " %" SCNx64 " %" SCNx64,
                &kind, &classes, &binding, &session, &generation) == 5 &&
                kind == DVM_SYSEVENT_VFS && generation >= 1 && generation <= QUEUE_REPORTS,
                "concurrent unexpected record");
        pthread_mutex_lock(&input->results->lock);
        input->results->seen[generation - 1]++;
        input->results->count++;
        pthread_mutex_unlock(&input->results->lock);
    }
}


/* EN/RU: Two native consumers must jointly receive each queued sequence exactly once.
 * Два native consumers совместно получают каждую sequence ровно один раз. */
static void competing_readers(DVM_SysEvents *events, DVM_VfsHandle *first, DVM_VfsHandle *second)
{
    Readers results = {PTHREAD_MUTEX_INITIALIZER, {0}, 0};
    Reader inputs[QUEUE_READERS] = {{first, &results}, {second, &results}};
    pthread_t threads[QUEUE_READERS];
    for (unsigned index = 1; index <= QUEUE_REPORTS; index++)
    {
        require(post(events, index), "concurrent fixture publication");
    }
    for (unsigned index = 0; index < QUEUE_READERS; index++)
    {
        require(!pthread_create(&threads[index], NULL, reader, &inputs[index]), "reader creation");
    }
    for (unsigned index = 0; index < QUEUE_READERS; index++)
    {
        require(!pthread_join(threads[index], NULL), "reader join");
    }
    require(results.count == QUEUE_REPORTS, "missing concurrent record");
    for (unsigned index = 0; index < QUEUE_REPORTS; index++)
    {
        require(results.seen[index] == 1, "duplicate/missing sequence");
    }
    pthread_mutex_destroy(&results.lock);
}


/* EN/RU: Full inbox plus allocator failure must still yield one final exit, then EOF.
 * Full inbox и allocation failure должны сохранить один final exit, затем EOF. */
static void retirement(DVM_SysEvents *events, DVM_VfsHandle *first, DVM_VfsHandle *second)
{
    Record record;
    for (unsigned index = 1; index < DVM_SYSEVENT_LIMIT; index++)
    {
        require(post(events, index), "retirement fixture publication");
    }
    short_read(first);
    fail_allocations = 1;
    require(!post(events, DVM_SYSEVENT_LIMIT), "retirement allocation injection");
    fail_allocations = 0;
    require(post(events, DVM_SYSEVENT_LIMIT) && events->pending == DVM_SYSEVENT_LIMIT, "refill before retirement");
    fail_allocations = 1;
    dvm_sysevents_retire(events);
    dvm_sysevents_retire(events);
    fail_allocations = 0;
    require(refused_allocations == 3 && events->exit_pending, "retirement allocated or lost exit");
    short_read(first);
    dvm_vfs_close(first);
    record = read_record(second);
    require(record.kind == DVM_SYSEVENT_RESYNC, "retirement lost resync");
    require(dvm_sysevents_open(events, "r", first) == FVM_OK, "retired inbox reopen");
    for (unsigned index = 1; index <= DVM_SYSEVENT_LIMIT; index++)
    {
        record = read_record(second);
        require(record.kind == DVM_SYSEVENT_VFS && record.generation == index, "retirement lost/reordered queue");
    }
    short_read(first);
    record = read_record(second);
    require(record.kind == DVM_SYSEVENT_EXIT, "retirement lost final exit");
    empty(first, 1);
    empty(second, 1);
}


/* EN/RU: Run queue scenarios before production-device identity is installed by main.
 * Проверить queue scenarios до установки production-device identity основным test. */
void test_sysevents_queue(void)
{
    DVM_Namespace *group = dvm_namespace_create("/", "/", NULL);
    DVM_SysEvents *events = dvm_sysevents_create(group);
    DVM_VfsHandle first = {0}, second = {0};
    Record initial;
    require(events && dvm_sysevents_open(events, "r", &first) == FVM_OK &&
            dvm_sysevents_open(events, "r", &second) == FVM_OK, "inbox/FD construction");
    initial = read_record(&first);
    require(initial.kind == DVM_SYSEVENT_NAMESPACE, "initial namespace report");
    shared_head(events, &first, &second);
    overflow(events, &first, &second);
    allocation_failure(events, &first);
    competing_readers(events, &first, &second);
    retirement(events, &first, &second);
    dvm_vfs_close(&first);
    dvm_vfs_close(&second);
    dvm_sysevents_release(events);
    dvm_namespace_release(group);
    puts("PASS inbox atomic head/close/reopen/readers, single resync, allocator failure and guaranteed final exit");
}
