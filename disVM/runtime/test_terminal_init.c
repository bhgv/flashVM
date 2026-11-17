/* Copyright (c) 2026 bhgv
 * EN: Native launch handoff feeding real guest terminal FD operations and live stream discovery.
 * RU: Native launch handoff для настоящих guest FD operations и live stream discovery.
 */
#include "../native/tests/launch/backend.h"
#include "vfs/launch/service.h"
#include "vfs/export.h"
static LaunchState terminal; /* Root storage survives all guest/native owners. */
static LaunchState execution; /* Provider-side environment root storage. */
static unsigned opened; /* Successfully acquired independent native handles. */
static unsigned closed; /* Consumed native handles, checked after real shutdown. */
static unsigned retired; /* Final channel provider retirement count. */
static int offered; /* Dynamic extra channel existence, changed by control write. */
void __real_dvm_namespace_boot_shutdown(void);


/* EN/RU: One independent bounded byte stream; native fixture storage outlives all borrowers.
 * Один bounded byte stream; native fixture storage живёт дольше borrowers. */
typedef struct Bytes
{
    unsigned char data[8192]; /* Control/bulk bytes never share storage. */
    size_t used; /* Confirmed write prefix. */
    size_t read; /* Acknowledged receive prefix. */
} Bytes;
static Bytes control; /* Main duplex connection or receive/send pair. */
static Bytes bulk; /* Dynamically offered independent stream. */


/* EN/RU: Live channel metadata, no tree snapshot; borrowed arguments, ordinary port status.
 * Live metadata каналов без tree snapshot; borrowed arguments, port status. */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *out)
{
    (void)state;
    int directory = !strcmp(path, ".");
    if (!directory && strcmp(path, "ctl") && strcmp(path, "rx") && strcmp(path, "tx") &&
        (!offered || strcmp(path, "streams/17")))
    {
        return FVM_NOT_FOUND;
    }
    memset(out, 0, sizeof(*out));
    out->directory = directory;
    out->mode = directory ? 0777 : 0666;
    strcpy(out->name, directory ? "link" : "channel");
    return FVM_OK;
}


/* EN/RU: Fragment native receives to at most3bytes; preserve actual progress and no record assumptions.
 * Fragment read до3bytes; actual progress без предположений о границах records. */
static FVM_Result read_bytes(void *state, void *output, size_t size, size_t *received)
{
    Bytes *bytes = state;
    size_t count = bytes->used-bytes->read;
    if (count > size)
    {
        count = size;
    }
    if (count > 3)
    {
        count = 3;
    }
    memcpy(output, bytes->data+bytes->read, count);
    bytes->read += count;
    *received = count;
    return FVM_OK;
}


/* EN/RU: Echo arbitrary borrowed bytes; control offer publishes one extra live node and its name.
 * Echo borrowed bytes; control offer публикует extra live node и возвращает его имя. */
static FVM_Result write_bytes(void *state, const void *input, size_t size)
{
    Bytes *bytes = state;
    if (bytes == &control && size == 5 && !memcmp(input, "offer", 5))
    {
        offered = 1;
        input = "streams/17";
        size = 10;
    }
    if (size > sizeof(bytes->data)-bytes->used)
    {
        return FVM_ERROR;
    }
    memcpy(bytes->data+bytes->used, input, size);
    bytes->used += size;
    return FVM_OK;
}


/* EN/RU: Count consuming close; outer factory wrapper keeps native storage pinned.
 * Учесть consuming close; outer wrapper удерживает native storage. */
static void close_bytes(void *state)
{
    (void)state;
    closed++;
}
static const DVM_VfsOps stream = {NULL, read_bytes, write_bytes, NULL, NULL, close_bytes}; /* Native byte streams. */


/* EN/RU: Open current channel only, respecting live availability; output owns one stream on success.
 * Открыть текущий канал по live availability; output владеет stream при успехе. */
static FVM_Result open_bytes(void *state, const char *path, const char *mode, DVM_VfsHandle *out)
{
    FVM_FileInfo meta;
    launch_check(!strcmp(mode, "r") || !strcmp(mode, "wo") || !strcmp(mode, "rw"),
        "capability open requested create/truncate or incompatible mode");
    FVM_Result status = info(state, path, &meta);
    if (status != FVM_OK || meta.directory)
    {
        return status == FVM_OK ? FVM_ERROR : status;
    }
    out->ops = &stream;
    out->state = !strcmp(path, "streams/17") ? &bulk : &control;
    out->native_flags = DVM_VFS_CANCEL_DATA | DVM_VFS_CANCEL_CLOSE;
    opened++;
    return FVM_OK;
}


/* EN/RU: Last native channel provider reference; no guest/FlashVM heap access.
 * Последний native channel provider reference; без доступа к VM heaps. */
static void destroy(void *state)
{
    (void)state;
    retired++;
}
static const DVM_ProviderOps ops = {info, open_bytes, destroy, DVM_PROVIDER_STABLE_BIND}; /* Live native channel grant. */


/* EN/RU: Prepare/take native launch request before app init and expose its channel grant to capture.
 * Only this fixture chooses /link; production namespace naming is caller-controlled.
 * Подготовить/take launch до init; только fixture выбирает /link для capture capability. */
DVM_Namespace *__wrap_dvm_namespace_rebase(DVM_ResourcePlan *cwd)
{
    (void)cwd;
    terminal.terminal = 1;
    DVM_LaunchOptions options = launch_options(&terminal, &execution);
    DVM_TerminalChannels channels = {0};
    channels.provider = dvm_provider_create_native(&ops, NULL, NULL, 1, DVM_PROVIDER_CANCEL_PATH);
    strcpy(channels.receive, "rx");
    strcpy(channels.send, "tx");
    DVM_LaunchSpec spec = {0};
    spec.policy = options.policy;
    spec.terminal = options.terminal;
    spec.terminal_origin = options.terminal_origin;
    strcpy(spec.expected_provider, options.provider_origin.system);
    strcpy(spec.module, "terminal-app");
    spec.flashvm = &channels;
    DVM_LaunchRequest *request = NULL;
    DVM_LaunchTicket *ticket = dvm_launch_request_create(&spec, &request);
    launch_check(ticket && dvm_launch_request_prepare(request, options.provider, &options.provider_origin), "terminal launch prepare");
    DVM_LaunchHandoff *handoff = dvm_launch_request_take(request);
    DVM_NamespacePlan target;
    launch_check(handoff && dvm_namespace_plan(handoff->group, "/link", &target) &&
        dvm_namespace_mount_provider(handoff->group, &target, handoff->flashvm.provider, "/FlashVM") == 1,
        "channel grant exposure for actual guest capture");
    DVM_Namespace *group = handoff->group;
    handoff->group = NULL;
    dvm_launch_handoff_free(handoff);
    dvm_launch_ticket_free(ticket);
    dvm_launch_request_free(request);
    dvm_provider_release(channels.provider);
    dvm_provider_release(options.terminal);
    dvm_provider_release(options.provider);
    return group;
}


/* EN/RU: After real VM GC/close drain, all channel streams and grant roots must retire exactly once.
 * После VM GC/close drain каждый channel stream и grant root должен завершиться ровно один раз. */
void __wrap_dvm_namespace_boot_shutdown(void)
{
    __real_dvm_namespace_boot_shutdown();
    launch_check(opened >= 5 && opened == closed && retired == 1 &&
        terminal.destroyed == 1 && execution.destroyed == 1, "terminal FD/root lifetime drain");
    puts("PASS terminal init native lifetimes");
}
