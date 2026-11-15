/* Copyright (c) 2026 bhgv
 * EN: Private source/event projection for real guest watch and native mailbox tests.
 * RU: Private source/event projection для guest watch и native mailbox tests.
 */
#include "vfs/boot.h"
#include "vfs/eventfs.h"
#include "vfs/sysevents_internal.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: Native state shared only through its mutex; provider callback lifetime
 * pins the borrowed self pointer. Native state под mutex; callback удерживает self. */
typedef struct State
{
    pthread_mutex_t lock; /* Protects policy, value and source report counters. */
    DVM_Provider *self; /* Borrowed self identity, used only while a callback holds factory lifetime. */
    int allowed; /* Current event/data root READ permission. */
    char value; /* Source data reread by guest after notifications. */
    unsigned destroyed; /* Final factory cleanup count. */
    uint64_t session; /* Source's local journal epoch. */
    uint64_t generation; /* Source report sequence within epoch. */
} State;
static State source = {PTHREAD_MUTEX_INITIALIZER, NULL, 1, 'A', 0, 1, 1}; /* One source per private runtime process. */


/* EN/RU: Fail fixture with a native diagnostic; never export pointers to guest.
 * Завершить fixture с native diagnostic; pointers guest не передаются. */
static void require(int ok, const char *label)
{
    if (!ok)
    {
        fprintf(stderr, "provider watch: %s\n", label);
        abort();
    }
}


/* EN/RU: Gate only the source root READ used by event projection/data opens.
 * Проверять только source root READ, используемый projection/data opens. */
static FVM_Result access_check(void *state, const char *path, unsigned access)
{
    State *value = state;
    int allowed;
    require(!strcmp(path, ".") && access == FVM_ACCESS_READ, "unexpected source policy query");
    pthread_mutex_lock(&value->lock);
    allowed = value->allowed;
    pthread_mutex_unlock(&value->lock);
    return allowed ? FVM_OK : FVM_ERROR;
}


/* EN/RU: Return declared test source metadata, not a native host-file substitute.
 * Вернуть metadata объявленных source nodes, без host-file подмены. */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *output)
{
    (void)state;
    if (strcmp(path, ".") && strcmp(path, "data") && strcmp(path, "ctl") && strcmp(path, "ready"))
    {
        return FVM_NOT_FOUND;
    }
    memset(output, 0, sizeof(*output));
    output->directory = !strcmp(path, ".");
    output->mode = output->directory ? 0555 : !strcmp(path, "ctl") ? 0222 : 0444;
    strcpy(output->name, path);
    return FVM_OK;
}


/* EN/RU: Read current data byte or deterministic sleep-ready flag.
 * Прочитать data byte либо deterministic sleep-ready flag. No cursor effects. */
static void data_read(void *state, void *bytes, size_t size, DVM_IoResult *result)
{
    State *value = state;
    memset(result, 0, sizeof(*result));
    result->flags = DVM_IO_PREFIX;
    pthread_mutex_lock(&value->lock);
    result->status = value->allowed ? FVM_OK : FVM_ERROR;
    if (size && value->allowed)
    {
        *(char *)bytes = value->value;
        result->count = 1;
    }
    pthread_mutex_unlock(&value->lock);
}


/* EN/RU: Fixture control mutates source then publishes a typed journal report.
 * These controls are test-only, not a production guest policy/admin endpoint.
 * Test-only control меняет source и публикует report; не production admin endpoint. */
static void write_control(void *state, const void *bytes, size_t size, DVM_IoResult *result)
{
    State *value = state;
    DVM_ChangeEvent event;
    unsigned kinds = DVM_CHANGE_RIGHTS;
    int reconnect = size == 10 && !memcmp(bytes, "reconnect\n", size);
    memset(result, 0, sizeof(*result));
    result->flags = DVM_IO_PREFIX;
    result->status = FVM_OK;
    if (!size)
    {
        return;
    }
    pthread_mutex_lock(&value->lock);
    if (size == 7 && !memcmp(bytes, "change\n", size))
    {
        value->value = 'B';
        kinds |= DVM_CHANGE_CONTENT;
    }
    else if (size == 5 && !memcmp(bytes, "race\n", size))
    {
        value->value = 'C';
        kinds = DVM_CHANGE_CONTENT;
    }
    else if (size == 5 && !memcmp(bytes, "deny\n", size))
    {
        value->allowed = 0;
    }
    else if (size == 6 && !memcmp(bytes, "allow\n", size))
    {
        value->allowed = 1;
    }
    else if (size == 5 && !memcmp(bytes, "load\n", size))
    {
        kinds = DVM_CHANGE_LOAD | DVM_CHANGE_NETWORK;
    }
    else if (reconnect)
    {
        value->value = 'D';
        value->session = dvm_changes_reconnect(dvm_provider_changes(value->self), value->session);
        value->generation = 1;
        require(value->session != 0, "new source epoch");
    }
    else
    {
        result->status = FVM_INVALID_ARGUMENT;
    }
    event.session = value->session;
    event.generation = reconnect ? 1 : ++value->generation;
    event.kinds = kinds;
    event.flags = 0;
    event.resource = 0;
    pthread_mutex_unlock(&value->lock);
    if (result->status == FVM_OK)
    {
        if (!reconnect)
        {
            require(dvm_changes_notify(dvm_provider_changes(value->self), &event) == 1, "source event publish");
        }
        result->count = size;
    }
}


/* EN/RU: Stream state is borrowed from retained factory, no per-open allocation.
 * Stream state заимствован у retained factory; per-open allocation нет. */
static void close_stream(void *state)
{
    (void)state;
}
static const DVM_VfsOps data_ops = {NULL, NULL, NULL, NULL, NULL, close_stream, NULL, data_read, NULL}; /* Source data capability. */
static const DVM_VfsOps control_ops = {NULL, NULL, NULL, NULL, NULL, close_stream, NULL, NULL, write_control}; /* Private fixture control capability. */


/* EN/RU: Open only declared directions; readonly data enforces current policy.
 * Открывать declared directions; data проверяет current policy. */
static FVM_Result open_source(void *state, const char *path, const char *mode, DVM_VfsHandle *output)
{
    const DVM_VfsOps *ops;
    if (!strcmp(path, "ctl") && !strcmp(mode, "wo"))
    {
        ops = &control_ops;
    }
    else if (!strcmp(path, "data") && !strcmp(mode, "r") && access_check(state, ".", FVM_ACCESS_READ) == FVM_OK)
    {
        ops = &data_ops;
    }
    else
    {
        return FVM_UNSUPPORTED;
    }
    output->ops = ops;
    output->state = state;
    return FVM_OK;
}


/* EN/RU: Record final source cleanup after native/guest event observers retire.
 * Посчитать final source cleanup после native/guest observers. */
static void destroy(void *state)
{
    State *value = state;
    value->destroyed++;
}
static const DVM_ProviderOps source_ops = {info, open_source, destroy, DVM_PROVIDER_STABLE_BIND, access_check}; /* Fixed source identities. */
int __real_dvm_namespace_boot_initialize(const char *directory);
void __real_dvm_close_worker_stop(void);


/* EN/RU: Install source and optional event projection; normal /dev/sysevents is
 * installed by production boot code, never injected by this fixture.
 * Установить source/projection; /dev/sysevents устанавливает production boot. */
int __wrap_dvm_namespace_boot_initialize(const char *directory)
{
    DVM_Provider *projection;
    DVM_NamespacePlan target;
    if (!__real_dvm_namespace_boot_initialize(directory))
    {
        return 0;
    }
    source.self = dvm_provider_create(&source_ops, &source);
    projection = dvm_provider_eventfs(source.self);
    require(source.self && projection, "provider construction");
    require(dvm_namespace_plan(dvm_namespace_boot_group(), "view", &target) &&
        dvm_namespace_mount_provider(dvm_namespace_boot_group(), &target, source.self, "/watch-source") == 1,
        "source mount");
    require(dvm_namespace_plan(dvm_namespace_boot_group(), "watch", &target) &&
        dvm_namespace_mount_provider(dvm_namespace_boot_group(), &target, projection, "/event-projection") == 1,
        "projection mount");
    dvm_provider_release(projection);
    dvm_provider_release(source.self);
    return 1;
}


/* EN/RU: Verify final source ownership after all owners and descriptors retire.
 * Проверить final source ownership после retirement owners и descriptors. */
void __wrap_dvm_close_worker_stop(void)
{
    __real_dvm_close_worker_stop();
    require(source.destroyed == 1, "source lifetime");
    puts("PASS provider watch host source lifetime and automatic inbox retirement");
}
