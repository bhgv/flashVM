/* Real AVM1 stops controlled through bound binary requests, with two independent observers. */
#include "flashvm/rdebug.h"
#include "flashvm/rwire.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/jit.h"
#include "flashvm/execution_mode.h"
#include <stdio.h>
#include <string.h>

#define CHECK(x)                                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(x))                                                                                                      \
        {                                                                                                              \
            fprintf(stderr, "rdebug:%d %s\n", __LINE__, #x);                                                           \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)


typedef struct Host
{
    FVM_RDispatch *dispatch;
    FVM_RDebug *debug;
    FVM_RHandle resource;
    FVM_RWatch watchers[2];
    FVM_SBindings requests, events;
    FVM_SBinding request_slots[1], event_slots[1];
    unsigned long stop;
    int stage, abort_stop, failed;
} Host;


static size_t word(unsigned char *p, unsigned long n)
{
    FVM_SInteger value;
    size_t used;
    value.high = 0;
    value.low = n;
    value.negative = 0;
    if (fvm_sinteger_write(&value, p, FVM_SRECORD_INTEGER_MAX, &used) != FVM_S_OK)
    {
        return 0;
    }
    return used;
}


static int binding(FVM_SBindings *table, FVM_SBinding *slot, const char *name)
{
    FVM_SBinding b;
    unsigned char body[256], record[300];
    size_t size, n, used;
    memset(&b, 0, sizeof(b));
    b.tag = 3;
    b.version = 1;
    b.type_size = (unsigned int)strlen(name);
    memcpy(b.type, name, b.type_size);
    return fvm_sbindings_init(table, slot, 1) == FVM_S_OK &&
           fvm_sbindings_payload_write(&b, 1, body, sizeof(body), &size) == FVM_S_OK &&
           fvm_srecord_write(0, body, size, record, sizeof(record), &n) == FVM_S_OK &&
           fvm_sbindings_apply(table, record, n, &used) == FVM_S_OK && used == n;
}


static int
send(Host *h, unsigned long session, unsigned long id, unsigned long operation, const void *data, size_t size)
{
    FVM_RRequest q;
    unsigned char packet[1024];
    size_t bytes, used;
    memset(&q, 0, sizeof(q));
    q.session = 999;
    q.id = id;
    q.target = h->resource;
    q.operation = operation;
    q.data = data;
    q.size = size;
    return fvm_rrequest_record_write(3, &q, packet, sizeof(packet), &bytes) == FVM_S_OK &&
           fvm_rdispatch_post_record(h->dispatch, session, &h->requests, packet, bytes, &used) == FVM_R_OK &&
           used == bytes;
}


static int take(Host *h, unsigned long session, unsigned long id, FVM_RResult expected, FVM_RReply *out)
{
    FVM_RReply reply;
    if (fvm_rdispatch_take(h->dispatch, session, id, &reply) != FVM_R_OK || reply.result != expected)
    {
        return 0;
    }
    if (out)
    {
        *out = reply;
    }
    return 1;
}


static int event(Host *h, unsigned int reader, unsigned long kind, FVM_RDebugSnapshot *snapshot)
{
    FVM_REvent native, decoded;
    unsigned char packet[1024];
    size_t bytes, used;
    return fvm_rdispatch_event_take(h->dispatch, 11 + reader, h->watchers[reader], &native) == FVM_R_OK &&
           native.kind == kind && !native.missed &&
           fvm_revent_record_write(3, &native, packet, sizeof(packet), &bytes) == FVM_S_OK &&
           fvm_revent_record_read(&h->events, packet, bytes, &decoded, &used) == FVM_S_OK && used == bytes &&
           fvm_rdebug_snapshot_read(decoded.data, decoded.size, snapshot) == FVM_R_OK;
}


/* This deterministic host service queues native transport work instead of waiting for a socket. */
static FVM_Result service(FVM_RDebug *debug, void *raw)
{
    Host *h = (Host *)raw;
    FVM_RReply reply;
    FVM_RDebugSnapshot snapshot;
    FVM_RDebugFrame frame;
    unsigned char data[256];
    unsigned int i;
    size_t size;
#define REQUIRE(x)                                                                                                     \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(x))                                                                                                      \
        {                                                                                                              \
            fprintf(stderr, "service:%d %s\n", __LINE__, #x);                                                          \
            h->failed = 1;                                                                                             \
            return FVM_ERROR;                                                                                          \
        }                                                                                                              \
    } while (0)
    REQUIRE(debug == h->debug);
    if (h->stage == 0)
    {
        for (i = 0; i < 2; ++i)
        {
            REQUIRE(event(h, i, FVM_RDEBUG_STOP_EVENT, &snapshot));
            REQUIRE(snapshot.state == FVM_RDEBUG_STOPPED && snapshot.stop && snapshot.frames >= 1);
            REQUIRE(snapshot.offset == 0 && snapshot.line == 9 && !strcmp(snapshot.source, "resource-debug.as"));
            if (i)
            {
                REQUIRE(snapshot.stop == h->stop);
            }
            else
            {
                h->stop = snapshot.stop;
            }
        }
        REQUIRE(fvm_rdebug_destroy(&h->debug) == FVM_R_BUSY && h->debug == debug);
        if (h->abort_stop)
        {
            return FVM_ERROR;
        }
        REQUIRE(send(h, 11, 1, FVM_RDEBUG_SNAPSHOT, 0, 0));
        size = word(data, h->stop);
        size += word(data + size, 0);
        REQUIRE(send(h, 12, 1, FVM_RDEBUG_FRAME, data, size));
        REQUIRE(send(h, 11, 2, FVM_RDEBUG_CLAIM, 0, 0));
        REQUIRE(send(h, 13, 1, FVM_RDEBUG_CLAIM, 0, 0));
        size = word(data, h->stop);
        REQUIRE(send(h, 13, 2, FVM_RDEBUG_RESUME, data, size));
        size = word(data, 0);
        size += word(data + size, 2);
        size += word(data + size, 0);
        REQUIRE(send(h, 11, 3, FVM_RDEBUG_BREAKPOINT, data, size));
        size = word(data, h->stop - 1);
        REQUIRE(send(h, 11, 4, FVM_RDEBUG_RESUME, data, size));
        size = word(data, 0);
        size += word(data + size, 0);
        size += word(data + size, 10);
        memcpy(data + size, "<function>", 10);
        size += 10;
        REQUIRE(send(h, 11, 5, FVM_RDEBUG_BREAKPOINT, data, size));
        h->stage = 1;
        return FVM_OK;
    }
    REQUIRE(h->stage == 1);
    REQUIRE(take(h, 11, 1, FVM_R_OK, &reply));
    REQUIRE(fvm_rdebug_snapshot_read(reply.data, reply.size, &snapshot) == FVM_R_OK && snapshot.controller == 11 &&
            snapshot.stop == h->stop);
    REQUIRE(take(h, 12, 1, FVM_R_OK, &reply));
    REQUIRE(fvm_rdebug_frame_read(reply.data, reply.size, &frame) == FVM_R_OK &&
            !strcmp(frame.function, "<function>") && frame.offset == 0);
    REQUIRE(frame.engine == FVM_CRASH_ENGINE_INTERPRETER);
    REQUIRE(take(h, 11, 2, FVM_R_OK, 0) && take(h, 13, 1, FVM_R_CONFLICT, 0));
    REQUIRE(take(h, 13, 2, FVM_R_DENIED, 0) && take(h, 11, 3, FVM_R_INVALID, 0));
    REQUIRE(take(h, 11, 4, FVM_R_CONFLICT, 0) && take(h, 11, 5, FVM_R_OK, 0));
    size = word(data, h->stop);
    REQUIRE(send(h, 11, 6, FVM_RDEBUG_RESUME, data, size));
    REQUIRE(send(h, 11, 7, FVM_RDEBUG_RESUME, data, size));
    h->stage = 2;
    return FVM_OK;
#undef REQUIRE
}


int main(void)
{
    const unsigned char body[] = {0x96, 5, 0, 7, 42, 0, 0, 0, 0x3e, 0};
    FVM_Config config;
    FVM_Runtime *runtime;
    FVM_Context *context;
    FVM_Object *function;
    FVM_JitCode *code = 0;
    FVM_Value result;
    FVM_RDebugSnapshot snapshot, saved;
    FVM_RDebugFrame frame, oldframe;
    FVM_RReply reply;
    FVM_REvent discarded;
    FVM_RDebug *unchanged = (FVM_RDebug *)1;
    FVM_RHandle target = {77, 77};
    FVM_ExecutionMode modes[3] = {FVM_EXECUTION_INTERPRETER, FVM_EXECUTION_JIT, FVM_EXECUTION_MANUAL};
    Host host;
    size_t done, count, size;
    unsigned int mode, i;
    unsigned char data[512];
    memset(&host, 0, sizeof(host));
    CHECK(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK &&
          fvm_context_create(runtime, &context) == FVM_OK);
    CHECK(fvm_function_create_avm1(context, body, sizeof(body), 0, 0, 0, &function) == FVM_OK &&
          fvm_gc_pin(context, function) == FVM_OK);
    CHECK(fvm_rdispatch_create(0, &host.dispatch) == FVM_R_OK);
    CHECK(binding(&host.requests, host.request_slots, FVM_RWIRE_REQUEST_TYPE));
    CHECK(binding(&host.events, host.event_slots, FVM_RWIRE_EVENT_TYPE));
    CHECK(fvm_rdebug_create(context, host.dispatch, "/debug/ctl", service, &host, &host.debug, &host.resource) ==
          FVM_R_OK);
    CHECK(fvm_rdebug_create(context, host.dispatch, "/debug/ctl", service, &host, &unchanged, &target) ==
              FVM_R_CONFLICT &&
          unchanged == (FVM_RDebug *)1 && target.id == 77);
    CHECK(fvm_rdispatch_grant(host.dispatch, host.resource, 11, FVM_R_READ | FVM_R_CONTROL | FVM_R_SUBSCRIBE) ==
          FVM_R_OK);
    CHECK(fvm_rdispatch_grant(host.dispatch, host.resource, 12, FVM_R_READ | FVM_R_SUBSCRIBE) == FVM_R_OK);
    CHECK(fvm_rdispatch_grant(host.dispatch, host.resource, 13, FVM_R_READ | FVM_R_CONTROL) == FVM_R_OK);
    CHECK(fvm_rdispatch_subscribe(host.dispatch, 11, host.resource, host.watchers) == FVM_R_OK);
    CHECK(fvm_rdispatch_subscribe(host.dispatch, 12, host.resource, host.watchers + 1) == FVM_R_OK);
    CHECK(send(&host, 11, 99, FVM_RDEBUG_CLAIM, 0, 0));
    CHECK(fvm_rdispatch_pump(host.dispatch, 1, &done) == FVM_R_OK && done == 1 && take(&host, 11, 99, FVM_R_OK, 0));
    CHECK(fvm_debug_set_breakpoint_handler(context, fvm_rdebug_breakpoint, host.debug) == FVM_OK);
    CHECK(fvm_debug_map_action(context, "<function>", 0, "resource-debug.as", 9) == FVM_OK);
    for (mode = 0; mode < 3; ++mode)
    {
        FVM_ExecutionStats before, after;
        unsigned long reclaimed;
        if (mode == 2 && fvm_jit_available())
        {
            CHECK(fvm_jit_compile(body, sizeof(body), &code) == FVM_OK);
            CHECK(fvm_function_set_body(context, function, fvm_jit_body, code, 0) == FVM_OK);
        }
        CHECK(fvm_execution_set_mode(context, modes[mode], 0) == FVM_OK);
        CHECK(fvm_debug_set_breakpoint(context, "<function>", 0, 1) == FVM_OK);
        before.struct_size = sizeof(before);
        after.struct_size = sizeof(after);
        CHECK(fvm_execution_get_stats(context, &before) == FVM_OK);
        host.stage = 0;
        CHECK(fvm_function_invoke(context, function, 0, 0, 0, &result) == FVM_OK && result.number == 42 &&
              !host.failed && host.stage == 2);
        CHECK(take(&host, 11, 6, FVM_R_OK, 0) && take(&host, 11, 7, FVM_R_CONFLICT, 0));
        for (i = 0; i < 2; ++i)
        {
            CHECK(event(&host, i, FVM_RDEBUG_RESUME_EVENT, &snapshot) && snapshot.state == FVM_RDEBUG_RUNNING &&
                  snapshot.stop == host.stop);
        }
        CHECK(fvm_debug_frame_count(context, &count) == FVM_NOT_FOUND);
        CHECK(fvm_execution_get_stats(context, &after) == FVM_OK && after.native_entries == before.native_entries &&
              after.interpreter_entries > before.interpreter_entries);
        before = after;
        CHECK(fvm_function_invoke(context, function, 0, 0, 0, &result) == FVM_OK && result.number == 42);
        CHECK(fvm_execution_get_stats(context, &after) == FVM_OK);
        if (mode && fvm_jit_available())
        {
            CHECK(after.native_entries == before.native_entries + 1);
        }
        CHECK(fvm_gc_collect(context, &reclaimed) == FVM_OK);

        /* Cached frame copies remain readable after callback release. */
        size = word(data, host.stop);
        size += word(data + size, 0);
        CHECK(send(&host, 12, 88, FVM_RDEBUG_FRAME, data, size));
        CHECK(fvm_rdispatch_pump(host.dispatch, 1, &done) == FVM_R_OK && take(&host, 12, 88, FVM_R_OK, &reply));
        CHECK(fvm_rdebug_frame_read(reply.data, reply.size, &frame) == FVM_R_OK);
        memset(&oldframe, 0xa5, sizeof(oldframe));
        frame = oldframe;
        data[0] = 99;
        CHECK(fvm_rdebug_frame_read(data, 1, &frame) == FVM_R_INVALID && !memcmp(&frame, &oldframe, sizeof(frame)));
    }
    host.stage = 0;
    host.abort_stop = 1;
    CHECK(fvm_debug_set_breakpoint(context, "<function>", 0, 1) == FVM_OK);
    CHECK(fvm_function_invoke(context, function, 0, 0, 0, &result) != FVM_OK && !host.failed);
    for (i = 0; i < 2; ++i)
    {
        CHECK(event(&host, i, FVM_RDEBUG_ABORT_EVENT, &snapshot) && snapshot.state == FVM_RDEBUG_ABORTED);
    }
    CHECK(fvm_rdebug_revoke(host.debug, 11) == FVM_R_OK);
    CHECK(fvm_rdispatch_event_take(host.dispatch, 11, host.watchers[0], &discarded) == FVM_R_UNKNOWN);
    CHECK(send(&host, 13, 90, FVM_RDEBUG_CLAIM, 0, 0));
    CHECK(send(&host, 12, 91, FVM_RDEBUG_SNAPSHOT, 0, 0));
    CHECK(fvm_rdispatch_pump(host.dispatch, 2, &done) == FVM_R_OK && take(&host, 13, 90, FVM_R_OK, 0) &&
          take(&host, 12, 91, FVM_R_OK, &reply));
    CHECK(fvm_rdebug_snapshot_read(reply.data, reply.size, &snapshot) == FVM_R_OK && snapshot.controller == 13);
    memset(&saved, 0xa5, sizeof(saved));
    snapshot = saved;
    for (i = 0; i < FVM_RDEBUG_SNAPSHOT_FIELDS; ++i)
    {
        CHECK(fvm_rdebug_snapshot_read(reply.data, i, &snapshot) == FVM_R_INVALID &&
              !memcmp(&snapshot, &saved, sizeof(snapshot)));
    }
    memcpy(data, reply.data, reply.size);
    data[0] = 0x82;
    CHECK(fvm_rdebug_snapshot_read(data, reply.size, &snapshot) == FVM_R_INVALID &&
          !memcmp(&snapshot, &saved, sizeof(snapshot)));
    CHECK(fvm_debug_set_breakpoint_handler(context, 0, 0) == FVM_OK);
    CHECK(send(&host, 12, 92, FVM_RDEBUG_SNAPSHOT, 0, 0));
    CHECK(fvm_rdebug_destroy(&host.debug) == FVM_R_OK && !host.debug);
    CHECK(fvm_rdispatch_pump(host.dispatch, 1, &done) == FVM_R_OK && take(&host, 12, 92, FVM_R_UNKNOWN, 0));
    CHECK(fvm_rdispatch_event_take(host.dispatch, 12, host.watchers[1], &discarded) == FVM_R_UNKNOWN);
    CHECK(fvm_rdispatch_destroy(&host.dispatch) == FVM_R_OK);
    CHECK(fvm_gc_unpin(context, function) == FVM_OK);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    if (code)
    {
        CHECK(fvm_jit_destroy(code) == FVM_OK);
    }
    puts("PASS binary debugger commands, independent stop/frame observers, controller arbitration, "
         "resume epochs and interpreter/JIT fallback");
    return 0;
}
