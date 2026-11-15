/* Copyright (c) 2026 bhgv
 * EN: Real provider chunk hints, selective reread, epoch recovery and lifetime.
 * RU: Настоящие provider chunk hints, selective reread, epoch recovery и lifetime.
 */
#include "vfs/boot.h"
#include "vfs/resource.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static DVM_Provider *retained; /* Native test owner pins the factory until all refreshes finish. */
static DVM_ChangeCursor cursor; /* One controller's private refresh watermark. */
static unsigned reads; /* Guest-native actual stream calls, before retirement. */
static unsigned probes; /* Diagnostic handle stat calls; must remain zero for success. */
static unsigned queries; /* Actual factory state rereads, including ordinary path metadata. */
static unsigned refreshes; /* Explicit dirty-state callbacks only. */
static unsigned closes; /* Close-worker counter, inspected after join. */
static unsigned destroyed; /* Final factory cleanup count. */
static int offline; /* Controller-owned simulated failed state query after retirement. */
int __real_dvm_namespace_boot_initialize(const char *directory);
void __real_dvm_close_worker_stop(void);


/* EN/RU: Fail the host fixture with a precise label; no guest pointers.
 * Завершить host fixture с точным label; без guest pointers. */
static void require(int ok, const char *label)
{
    if (!ok)
    {
        fprintf(stderr, "provider events: %s\n", label);
        abort();
    }
}


/* EN/RU: Handle info is diagnostic; no successful chunk should invoke it.
 * Handle info — diagnostic; успешный chunk не должен его вызывать. */
static FVM_Result stream_info(void *state, FVM_FileInfo *output)
{
    (void)state;
    probes++;
    memset(output, 0, sizeof(*output));
    return FVM_OK;
}


/* EN/RU: Emit ABC and current scoped hints, including a stale repeated report.
 * Выдать ABC и scoped hints, включая stale repeated report. No metadata I/O. */
static void stream_read(void *state, void *bytes, size_t size, DVM_IoResult *result)
{
    (void)state;
    require(size == 1 && reads < 3, "guest I/O replay");
    memset(result, 0, sizeof(*result));
    result->status = FVM_OK;
    result->count = 1;
    result->flags = DVM_IO_PREFIX | DVM_IO_CHANGED;
    result->session = 1;
    result->generation = reads == 1 ? 3 : 2;
    result->change_kinds = reads == 1 ? DVM_CHANGE_NAMESPACE : DVM_CHANGE_RIGHTS | DVM_CHANGE_CONTENT;
    result->change_flags = DVM_CHANGE_RESOURCE;
    result->resource = 7;
    *(char *)bytes = (char)('A' + reads++);
}


/* EN/RU: One native stream closes after every guest borrower retires.
 * Один native stream закрывается после завершения guest borrowers. */
static void stream_close(void *state)
{
    (void)state;
    closes++;
}
static const DVM_VfsOps stream_ops = {stream_info, NULL, NULL, NULL, NULL, stream_close, NULL, stream_read, NULL}; /* Precise-only read with hints. */


/* EN/RU: Real reread target; temporary offline refusal cannot acknowledge events.
 * Настоящая цель reread; offline refusal не подтверждает events. */
static FVM_Result factory_info(void *state, const char *path, FVM_FileInfo *output)
{
    (void)state;
    queries++;
    if (offline)
    {
        return FVM_ERROR;
    }
    memset(output, 0, sizeof(*output));
    output->directory = !strcmp(path, ".");
    output->mode = 0555;
    return FVM_OK;
}


/* EN/RU: Open one immutable test stream through the ordinary production factory.
 * Открыть test stream через ordinary production factory; no allocation needed. */
static FVM_Result factory_open(void *state, const char *path, const char *mode, DVM_VfsHandle *output)
{
    if (strcmp(path, "stream") || strcmp(mode, "r"))
    {
        return FVM_UNSUPPORTED;
    }
    output->ops = &stream_ops;
    output->state = state;
    return FVM_OK;
}


/* EN/RU: Final factory cleanup follows both stream close and refresh owner release.
 * Final cleanup следует stream close и освобождению refresh owner. */
static void factory_destroy(void *state)
{
    (void)state;
    require(closes == 1, "destroy before stream close");
    destroyed++;
}
static const DVM_ProviderOps factory = {factory_info, factory_open, factory_destroy, 0, NULL}; /* Trusted native fixture factory. */


/* EN/RU: Query actual factory metadata once, optionally inject another category
 * while refresh is running. Caller owns action and staged local output.
 * Запросить factory metadata, опционально добавить событие во время refresh. */
static FVM_Result reread(const DVM_ChangeBatch *ticket, void *argument)
{
    FVM_FileInfo metadata;
    int action = *(int *)argument;
    FVM_Result status;
    refreshes++;
    status = dvm_provider_info(retained, ".", &metadata);
    if (status != FVM_OK)
    {
        return status;
    }
    require(metadata.directory && metadata.mode == 0555, "wrong refreshed state");
    if (action)
    {
        DVM_ChangeEvent event = {ticket->event.session, ticket->event.generation + 1,
                                DVM_CHANGE_LOAD | DVM_CHANGE_NETWORK, 0, 0};
        require(dvm_changes_notify(dvm_provider_changes(retained), &event) == 1, "refresh race notification");
    }
    return FVM_OK;
}


/* EN/RU: Install provider and refresh the first observer before any guest I/O.
 * Установить provider, обновить initial observer до guest I/O. */
int __wrap_dvm_namespace_boot_initialize(const char *directory)
{
    static int state; /* Non-NULL stable fixture identity, outlives the factory. */
    DVM_NamespacePlan target;
    int action = 0;
    if (!__real_dvm_namespace_boot_initialize(directory))
    {
        return 0;
    }
    retained = dvm_provider_create(&factory, &state);
    require(retained && dvm_namespace_plan(dvm_namespace_boot_group(), "view", &target) &&
            dvm_namespace_mount_provider(dvm_namespace_boot_group(), &target, retained, "/event-provider") == 1, "mount");
    require(dvm_changes_refresh(dvm_provider_changes(retained), &cursor, DVM_CHANGE_ALL, reread, &action) == FVM_OK,
            "initial observer refresh");
    return 1;
}


/* EN/RU: After guest/close join, consume actual hints and exercise failed reread,
 * reconnection and independent observers. Retain factory through every callback.
 * После join проверить hints, failed reread, reconnect и независимого observer. */
void __wrap_dvm_close_worker_stop(void)
{
    DVM_Changes *changes;
    DVM_ChangeBatch batch;
    DVM_ChangeCursor other = {0}, before;
    DVM_ChangeEvent event = {1, 5, DVM_CHANGE_CONNECTION, 0, 0};
    int action = 1;
    __real_dvm_close_worker_stop();
    changes = dvm_provider_changes(retained);
    require(reads == 3 && closes == 1 && !probes && !destroyed, "success path probe/effect/lifetime");
    require(dvm_changes_pending(changes, &cursor, DVM_CHANGE_ALL, &batch) == 1 &&
            batch.event.kinds == (DVM_CHANGE_RIGHTS | DVM_CHANGE_NAMESPACE | DVM_CHANGE_CONTENT) &&
            batch.event.flags == DVM_CHANGE_RESOURCE && batch.event.resource == 7, "chunk hint routing/coalescing");
    require(dvm_changes_refresh(changes, &cursor, DVM_CHANGE_ALL, reread, &action) == FVM_OK, "first reread");
    require(dvm_changes_pending(changes, &cursor, DVM_CHANGE_ALL, &batch) == 1 &&
            batch.event.kinds == (DVM_CHANGE_LOAD | DVM_CHANGE_NETWORK), "racing categories lost");
    action = 0;
    require(dvm_changes_refresh(changes, &cursor, DVM_CHANGE_ALL, reread, &action) == FVM_OK &&
            dvm_changes_refresh(changes, &cursor, DVM_CHANGE_ALL, reread, &action) == FVM_OK && refreshes == 3,
            "clean observer reread provider");
    require(dvm_changes_pending(changes, &other, DVM_CHANGE_ALL, &batch) == 1, "other observer cleared");
    require(dvm_changes_notify(changes, &event) == 1, "disconnect hint");
    offline = 1;
    before = cursor;
    require(dvm_changes_refresh(changes, &cursor, DVM_CHANGE_ALL, reread, &action) == FVM_ERROR &&
            !memcmp(&before, &cursor, sizeof(cursor)), "failed reread acknowledged disconnect");
    require(dvm_changes_reconnect(changes, 1) == 2, "new session");
    offline = 0;
    require(dvm_changes_refresh(changes, &cursor, DVM_CHANGE_ALL, reread, &action) == FVM_OK, "recovered reread");
    event.generation = 100;
    require(dvm_changes_notify(changes, &event) == 0 &&
            dvm_changes_pending(changes, &cursor, DVM_CHANGE_ALL, &batch) == 0, "old session invalidated new state");
    require(refreshes == 5 && queries == 6, "reread counts");
    dvm_provider_release(retained);
    retained = NULL;
    require(destroyed == 1, "factory retained forever");
    puts("PASS provider event bridge/reread/race/disconnect/reconnect/independent observers and zero success stat");
}
