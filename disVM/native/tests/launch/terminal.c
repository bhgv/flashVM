/* Copyright (c) 2026 bhgv
 * EN: Real S-record bytes through launch-owned duplex/pair and dynamically offered bulk channels.
 * RU: Настоящие S-record bytes через launch-owned duplex/пару и динамически выданный bulk канал.
 */
#include "backend.h"
#include "../../../runtime/vfs/launch/service.h"
#include "flashvm/srecord.h"


typedef struct Connection Connection; /* Owner of two independent native streams. */


/* EN/RU: One bounded stream, borrowing connection storage through its provider pin.
 * Один bounded stream, storage удерживается provider pin. */
typedef struct Bytes
{
    Connection *owner; /* Borrowed containing connection through final close. */
    unsigned char bytes[8192]; /* Confirmed bytes, no record-boundary assumptions. */
    size_t size; /* Committed prefix. */
    size_t offset; /* Consumed prefix. */
} Bytes;


/* EN/RU: Native connection fixture outside both VM heaps; caller owns storage.
 * Native connection fixture вне VM heaps; caller владеет storage. */
struct Connection
{
    Bytes control; /* Duplex control or paired control stream. */
    Bytes data; /* Independently offered bulk stream. */
    unsigned opens; /* Successful channel acquisitions. */
    unsigned closes; /* Consumed independent handles. */
    unsigned destroyed; /* Final provider destruction count. */
    int bulk; /* New live node advertised after control exchange. */
    int denied; /* Dynamic server refusal, never an ambient fallback. */
};


/* EN/RU: Reject metadata here; channel opens do not need speculative stat operations.
 * Metadata здесь недоступна; channel open не требует предварительного stat. */
static FVM_Result info(void *state, const char *path, FVM_FileInfo *out)
{
    (void)state;
    (void)path;
    (void)out;
    return FVM_UNSUPPORTED;
}


/* EN/RU: Borrow state/output through one native read, intentionally fragmenting to one byte.
 * Borrow state/output на один native read, намеренно вернуть максимум один byte. */
static FVM_Result read_bytes(void *opaque, void *out, size_t size, size_t *received)
{
    Bytes *connection = opaque;
    *received = size && connection->offset < connection->size;
    if (*received)
    {
        *(unsigned char *)out = connection->bytes[connection->offset++];
    }
    return FVM_OK;
}


/* EN/RU: Append borrowed binary input exactly once, or fail before effects on capacity refusal.
 * Добавить borrowed binary input ровно один раз, capacity failure — до effects. */
static FVM_Result write_bytes(void *opaque, const void *bytes, size_t size)
{
    Bytes *connection = opaque;
    if (size > sizeof(connection->bytes)-connection->size)
    {
        return FVM_ERROR;
    }
    memcpy(connection->bytes+connection->size, bytes, size);
    connection->size += size;
    return FVM_OK;
}


/* EN/RU: Retire one native stream; provider wrapper retains the connection through close.
 * Завершить один native stream; provider wrapper удерживает connection до close. */
static void close_bytes(void *opaque)
{
    Bytes *bytes = opaque;
    bytes->owner->closes++;
}
static const DVM_VfsOps stream = {NULL, read_bytes, write_bytes, NULL, NULL, close_bytes}; /* Raw native bytes. */


/* EN/RU: Open duplex dot, paired ports, or a newly advertised bulk node; no heap pointers.
 * Открыть duplex dot, pair ports или новый bulk node; без heap pointers. */
static FVM_Result open_bytes(void *opaque, const char *path, const char *mode, DVM_VfsHandle *out)
{
    Connection *connection = opaque;
    (void)mode;
    if (connection->denied)
    {
        return FVM_ERROR;
    }
    if (strcmp(path, ".") && strcmp(path, "rx") && strcmp(path, "tx") &&
        (strcmp(path, "streams/17") || !connection->bulk))
    {
        return FVM_NOT_FOUND;
    }
    connection->opens++;
    out->ops = &stream;
    Bytes *bytes = !strcmp(path, "streams/17") ? &connection->data : &connection->control;
    bytes->owner = connection;
    out->state = bytes;
    out->native_flags = DVM_VFS_CANCEL_DATA | DVM_VFS_CANCEL_CLOSE;
    return FVM_OK;
}


/* EN/RU: Record final connection retirement; storage remains owned by the test caller.
 * Учесть final connection retirement; storage принадлежит test caller. */
static void destroy_connection(void *opaque)
{
    Connection *connection = opaque;
    connection->destroyed++;
}
static const DVM_ProviderOps ports = {info, open_bytes, destroy_connection, DVM_PROVIDER_STABLE_BIND}; /* Anchored channel namespace. */


/* EN/RU: Roundtrip two coalesced binary records through actual channel callbacks and codec.
 * Read fragments are independent of record boundaries; no new wire format or graphics adapter.
 * Передать две coalesced binary records через channel callbacks/codec; границы read независимы. */
static void records(DVM_VfsHandle *input, DVM_VfsHandle *output)
{
    unsigned char payload[2048], encoded[4096], received[4096];
    size_t first = 0, second = 0;
    for (size_t i = 0; i < sizeof(payload); i++)
    {
        payload[i] = (unsigned char)i;
    }
    launch_check(fvm_srecord_write(3, payload, sizeof(payload), encoded, sizeof(encoded), &first) == FVM_S_OK &&
        fvm_srecord_write(7, "bulk", 4, encoded+first, sizeof(encoded)-first, &second) == FVM_S_OK,
        "existing FlashVM codec encode");
    launch_check(dvm_vfs_write(output, encoded, first+second) == FVM_OK, "raw channel write");
    for (size_t i = 0; i < first+second; i++)
    {
        size_t count = 0;
        launch_check(dvm_vfs_read(input, received+i, 1, &count) == FVM_OK && count == 1,
            "fragmented channel byte read");
    }
    FVM_SRecord record;
    size_t used = 0;
    launch_check(fvm_srecord_read(received, first+second, 4096, &record, &used) == FVM_S_OK &&
        used == first && record.tag == 3 && record.size == sizeof(payload) &&
        !memcmp(record.payload, payload, sizeof(payload)), "first coalesced S-record");
    launch_check(fvm_srecord_read(received+used, second, 4096, &record, &used) == FVM_S_OK &&
        used == second && record.tag == 7 && record.size == 4 && !memcmp(record.payload, "bulk", 4),
        "second independent S-record");
}


/* EN/RU: Submit one channel grant through service/preparation/take, then retire all request owners.
 * Return owned handoff, caller still owns options/channel source pins.
 * Submit grant через service/prepare/take, убрать request owners; вернуть owned handoff. */
static DVM_LaunchHandoff *launch(DVM_LaunchOptions *options, DVM_TerminalChannels *channels)
{
    DVM_LaunchService *service = dvm_launch_service_create(options->provider_origin.system, 1);
    DVM_LaunchEndpoint *endpoint = dvm_launch_service_endpoint(service);
    DVM_LaunchSpec spec = {0};
    spec.policy = options->policy;
    spec.terminal = options->terminal;
    spec.terminal_origin = options->terminal_origin;
    strcpy(spec.expected_provider, options->provider_origin.system);
    strcpy(spec.module, "remote-entry");
    DVM_TerminalChannels invalid = *channels;
    memset(invalid.receive, 'x', sizeof(invalid.receive));
    spec.flashvm = &invalid;
    launch_check(!dvm_launch_service_submit(endpoint, &spec), "unterminated grant accepted");
    strcpy(invalid.receive, "../escape");
    launch_check(!dvm_launch_service_submit(endpoint, &spec), "escaping grant accepted");
    DVM_LaunchServiceInfo counters;
    launch_check(dvm_launch_service_info(service, &counters) && !counters.outstanding && !counters.queued,
        "invalid channel grant leaked admission quota");
    spec.flashvm = channels;
    DVM_LaunchTicket *ticket = dvm_launch_service_submit(endpoint, &spec);
    DVM_LaunchReview *review = dvm_launch_service_next(service);
    DVM_LaunchRequest *request = dvm_launch_review_request(review);
    DVM_LaunchDescription description;
    launch_check(ticket && request && dvm_launch_describe(request, &description) && description.terminal_channels,
        "channel grant lost at admission");
    strcpy(channels->receive, "caller-mutated");
    launch_check(dvm_launch_request_prepare(request, options->provider, &options->provider_origin), "channel launch prepare");
    DVM_LaunchHandoff *handoff = dvm_launch_request_take(request);
    launch_check(handoff && handoff->flashvm.provider && !dvm_launch_request_take(request), "one-use channel handoff");
    dvm_launch_service_close(service);
    dvm_launch_ticket_free(ticket);
    dvm_launch_review_free(review);
    dvm_launch_endpoint_free(endpoint);
    dvm_launch_service_free(service);
    return handoff;
}


/* EN/RU: Cover duplex/pair lifetime, live bulk, path escape refusal, denial and native pre-cancel.
 * Покрыть duplex/pair lifetime, live bulk, escape refusal, denial и native pre-cancel. */
int main(void)
{
    for (unsigned pair = 0; pair < 2; pair++)
    {
        Connection connection = {0};
        LaunchState terminal = {0}, provider = {0};
        terminal.terminal = 1;
        DVM_LaunchOptions options = launch_options(&terminal, &provider);
        DVM_TerminalChannels channels = {0};
        channels.provider = dvm_provider_create_native(&ports, &connection, NULL, 1, DVM_PROVIDER_CANCEL_PATH);
        strcpy(channels.receive, pair ? "rx" : ".");
        strcpy(channels.send, pair ? "tx" : ".");
        launch_check(dvm_terminal_channels_valid(&channels), "duplex/pair grant validation");
        DVM_LaunchHandoff *handoff = launch(&options, &channels);
        launch_check(!connection.opens, "channel opened before handoff consumer requested it");
        dvm_provider_release(channels.provider);
        launch_check(!connection.destroyed && !strcmp(handoff->flashvm.receive, pair ? "rx" : "."),
            "copied channel paths or transferred pin lost");
        launch_check(!connection.opens, "submission/preparation opened a channel before handoff");
        DVM_VfsHandle input = {0}, output = {0}, bulk = {0};
        if (pair)
        {
            launch_check(dvm_terminal_channel_open(&handoff->flashvm, handoff->flashvm.receive, "r", &input) == FVM_OK &&
                dvm_terminal_channel_open(&handoff->flashvm, handoff->flashvm.send, "w", &output) == FVM_OK,
                "paired channel acquisition");
            records(&input, &output);
        }
        else
        {
            launch_check(dvm_terminal_channel_open(&handoff->flashvm, ".", "r+", &input) == FVM_OK,
                "single duplex acquisition");
            records(&input, &input);
        }
        launch_check(dvm_terminal_channel_open(&handoff->flashvm, "streams/17", "w", &bulk) == FVM_NOT_FOUND,
            "unadvertised bulk node opened");
        connection.bulk = 1;
        launch_check(dvm_terminal_channel_open(&handoff->flashvm, "streams/17", "r+", &bulk) == FVM_OK,
            "new live bulk node invisible");
        size_t control_bytes = connection.control.size;
        records(&bulk, &bulk);
        launch_check(connection.control.size == control_bytes && connection.data.size > 2048,
            "bulk bytes mixed into control stream");
        unsigned opens = connection.opens;
        DVM_VfsHandle rejected = {0};
        const char *bad[] = {"", "/tx", "../tx", "a/../tx", "a//tx", "a/./tx", "tx/"};
        for (unsigned i = 0; i < sizeof(bad)/sizeof(bad[0]); i++)
        {
            launch_check(dvm_terminal_channel_open(&handoff->flashvm, bad[i], "w", &rejected) == FVM_INVALID_ARGUMENT,
                "channel grant escape accepted");
        }
        DVM_Cancel *cancel = dvm_cancel_create();
        DVM_CancelScope scope = {0};
        launch_check(cancel && dvm_cancel_enter(&scope, cancel), "channel cancellation scope");
        dvm_cancel_request(cancel);
        launch_check(dvm_terminal_channel_open(&handoff->flashvm, "tx", "w", &rejected) != FVM_OK &&
            dvm_cancel_leave(&scope), "pre-cancel channel acquisition");
        dvm_cancel_release(cancel);
        connection.denied = 1;
        launch_check(dvm_terminal_channel_open(&handoff->flashvm, "tx", "w", &rejected) == FVM_ERROR &&
            !rejected.ops && connection.opens == opens, "denied channel fallback or partial handle");
        dvm_launch_handoff_free(handoff);
        launch_check(!connection.destroyed, "open streams lost their channel provider pin");
        dvm_vfs_close(&input);
        dvm_vfs_close(&output);
        dvm_vfs_close(&bulk);
        launch_check(connection.destroyed == 1 && connection.closes == connection.opens, "channel lifetime leak");
        dvm_provider_release(options.terminal);
        dvm_provider_release(options.provider);
        launch_check(terminal.destroyed == 1 && provider.destroyed == 1, "launch root leak");
    }
    return 0;
}
