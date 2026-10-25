/* Real AVM1 stop/resume through two independent native 9P connections, no VM access by clients. */
#include "flashvm/r9p.h"
#include "flashvm/rdebug.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include <stdio.h>
#include <string.h>

#define CHECK(x)                                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(x))                                                                                                      \
        {                                                                                                              \
            fprintf(stderr, "9p-debug:%d %s\n", __LINE__, #x);                                                         \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)


typedef struct Client
{
    FVM_RFS *fs;
    FVM_R9P *server;
    unsigned char out[FVM_R9P_MSIZE];
    size_t size;
    FVM_SBindings events;
    FVM_SBinding event_slot;
    unsigned long event_offset;
    unsigned char waiting[4][FVM_R9P_MSIZE];
    size_t waiting_size[4];
} Client;


typedef struct Host
{
    FVM_RDispatch *dispatch;
    FVM_RDebug *debug;
    FVM_RHandle resource;
    Client clients[2];
    int stage, failed;
    unsigned long stop;
} Host;


static void put(unsigned char *p, unsigned long value, size_t n)
{
    size_t i;
    for (i = 0; i < n; ++i)
    {
        p[i] = (unsigned char)(value & 255);
        value >>= 8;
    }
}


static unsigned long get(const unsigned char *p, size_t n)
{
    unsigned long value = 0;
    size_t i;
    for (i = 0; i < n; ++i)
    {
        value |= (unsigned long)p[i] << (i * 8);
    }
    return value;
}


static size_t string(unsigned char *p, const char *s)
{
    size_t n = strlen(s);
    put(p, (unsigned long)n, 2);
    memcpy(p + 2, s, n);
    return n + 2;
}


static int send(Client *c, unsigned int type, unsigned int tag, const void *payload, size_t n, int reply)
{
    unsigned char input[FVM_R9P_MSIZE];
    size_t consumed;
    put(input, (unsigned long)(n + 7), 4);
    input[4] = (unsigned char)type;
    put(input + 5, tag, 2);
    if (n)
    {
        memcpy(input + 7, payload, n);
    }
    if (fvm_r9p_request(c->server, input, n + 7, c->out, sizeof(c->out), &c->size, &consumed) != FVM_R_OK ||
        consumed != n + 7)
    {
        return 0;
    }
    if (reply < 0)
    {
        return c->size == 0;
    }
    return c->size >= 7 && c->out[4] == reply && get(c->out + 5, 2) == tag;
}


static int poll(Client *c, unsigned int type, unsigned int tag)
{
    size_t i, free_slot;
    for (i = 0; i < 4; ++i)
    {
        if (c->waiting_size[i] && get(c->waiting[i] + 5, 2) == tag)
        {
            c->size = c->waiting_size[i];
            memcpy(c->out, c->waiting[i], c->size);
            c->waiting_size[i] = 0;
            return c->out[4] == type;
        }
    }
    for (;;)
    {
        if (fvm_r9p_poll(c->server, c->out, sizeof(c->out), &c->size) != FVM_R_OK || c->size < 7)
        {
            return 0;
        }
        if (get(c->out + 5, 2) == tag)
        {
            return c->out[4] == type;
        }
        for (free_slot = 0; free_slot < 4 && c->waiting_size[free_slot]; ++free_slot)
        {
        }
        if (free_slot == 4)
        {
            return 0;
        }
        memcpy(c->waiting[free_slot], c->out, c->size);
        c->waiting_size[free_slot] = c->size;
    }
}


static int walk(Client *c, unsigned long fid, const char *leaf, int full)
{
    unsigned char p[128];
    size_t n = 10;
    put(p, 0, 4);
    put(p + 4, fid, 4);
    put(p + 8, leaf ? 3 : 2, 2);
    n += string(p + n, "1.1");
    n += string(p + n, leaf ? "ops" : "events");
    if (leaf)
    {
        n += string(p + n, leaf);
    }
    return send(c, 110, 2, p, n, 111) && get(c->out + 7, 2) == (unsigned long)full;
}


static int open_file(Client *c, unsigned long fid, unsigned int mode)
{
    unsigned char p[5];
    put(p, fid, 4);
    p[4] = (unsigned char)mode;
    return send(c, 112, 3, p, sizeof(p), 113);
}


static int pending_read(Client *c, unsigned long fid, unsigned long offset, unsigned int tag)
{
    unsigned char p[16];
    memset(p, 0, sizeof(p));
    put(p, fid, 4);
    put(p + 4, offset, 4);
    put(p + 12, FVM_R9P_MSIZE, 4);
    return send(c, 116, tag, p, sizeof(p), -1);
}


static int command(Client *c, const char *operation, const void *data, size_t size, unsigned int tag)
{
    unsigned char p[FVM_R_BYTES + 16];
    if (!walk(c, 2, operation, 3) || !open_file(c, 2, 2))
    {
        return 0;
    }
    memset(p, 0, 16);
    put(p, 2, 4);
    put(p + 12, (unsigned long)size, 4);
    if (size)
    {
        memcpy(p + 16, data, size);
    }
    return send(c, 118, 4, p, size + 16, 119) && pending_read(c, 2, 0, tag);
}


static int reply(Client *c, unsigned int tag, FVM_RReply *out)
{
    FVM_SBindings bindings;
    FVM_SBinding slot;
    size_t used, n;
    unsigned long id;
    if (!poll(c, 117, tag) || c->size < 11 || get(c->out + 7, 4) != c->size - 11)
    {
        return 0;
    }
    n = c->size - 11;
    return fvm_sbindings_init(&bindings, &slot, 1) == FVM_S_OK &&
           fvm_sbindings_apply(&bindings, c->out + 11, n, &used) == FVM_S_OK &&
           fvm_rreply_record_read(&bindings, c->out + 11 + used, n - used, &id, out, &used) == FVM_S_OK &&
           out->result == FVM_R_OK;
}


static int event(Client *c, unsigned int tag, unsigned long kind, FVM_RDebugSnapshot *snapshot)
{
    FVM_REvent event;
    size_t used, n;
    const unsigned char *p;
    if (!poll(c, 117, tag) || c->size < 11 || get(c->out + 7, 4) != c->size - 11)
    {
        return 0;
    }
    n = c->size - 11;
    p = c->out + 11;
    if (!c->event_offset)
    {
        if (fvm_sbindings_apply(&c->events, p, n, &used) != FVM_S_OK)
        {
            return 0;
        }
        p += used;
        n -= used;
    }
    c->event_offset += (unsigned long)c->size - 11;
    return fvm_revent_record_read(&c->events, p, n, &event, &used) == FVM_S_OK && used == n && event.kind == kind &&
           !event.missed && fvm_rdebug_snapshot_read(event.data, event.size, snapshot) == FVM_R_OK;
}


static FVM_Result service(FVM_RDebug *debug, void *raw)
{
    Host *h = (Host *)raw;
    FVM_RDebugSnapshot snapshot;
    FVM_RReply result;
    unsigned char payload[32];
    size_t n, i;
    FVM_SInteger number;
    unsigned char clunk[4];
#define REQUIRE(x)                                                                                                     \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(x))                                                                                                      \
        {                                                                                                              \
            fprintf(stderr, "9p-service:%d %s\n", __LINE__, #x);                                                       \
            h->failed = 1;                                                                                             \
            return FVM_ERROR;                                                                                          \
        }                                                                                                              \
    } while (0)
    REQUIRE(debug == h->debug);
    if (!h->stage)
    {
        for (i = 0; i < 2; ++i)
        {
            REQUIRE(event(h->clients + i, 100 + (unsigned int)i, FVM_RDEBUG_STOP_EVENT, &snapshot));
            REQUIRE(snapshot.state == FVM_RDEBUG_STOPPED && snapshot.stop == 1 && snapshot.frames >= 1);
            REQUIRE(snapshot.line == 23 && !strcmp(snapshot.source, "ninep-debug.as"));
            h->stop = snapshot.stop;
        }
        REQUIRE(walk(h->clients + 1, 2, "6", 2)); /* Read-only observer gets only a partial walk. */
        REQUIRE(command(h->clients, "4", 0, 0, 110));
        h->stage = 1;
        return FVM_OK;
    }
    REQUIRE(h->stage == 1 && reply(h->clients, 110, &result));
    put(clunk, 2, 4);
    REQUIRE(send(h->clients, 120, 5, clunk, sizeof(clunk), 121));
    number.high = 0;
    number.low = h->stop;
    number.negative = 0;
    REQUIRE(fvm_sinteger_write(&number, payload, sizeof(payload), &n) == FVM_S_OK);
    payload[n++] = 0x80; /* Independent canonical zero frame index. */
    REQUIRE(command(h->clients + 1, "2", payload, n, 111));
    REQUIRE(fvm_sinteger_write(&number, payload, sizeof(payload), &n) == FVM_S_OK);
    REQUIRE(command(h->clients, "6", payload, n, 112));
    for (i = 0; i < 2; ++i)
    {
        REQUIRE(pending_read(h->clients + i, 1, h->clients[i].event_offset, 120 + (unsigned int)i));
    }
    h->stage = 2;
    return FVM_OK;
#undef REQUIRE
}


int main(void)
{
    const unsigned char body[] = {0x96, 5, 0, 7, 42, 0, 0, 0, 0x3e, 0};
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    FVM_Object *function = 0;
    FVM_Value result;
    FVM_RReply response;
    FVM_RDebugFrame frame;
    FVM_RDebugSnapshot snapshot;
    Host host;
    size_t i, n;
    unsigned char p[128];
    Client *c;
    memset(&host, 0, sizeof(host));
    CHECK(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK);
    CHECK(fvm_context_create(runtime, &context) == FVM_OK);
    CHECK(fvm_function_create_avm1(context, body, sizeof(body), 0, 0, 0, &function) == FVM_OK);
    CHECK(fvm_gc_pin(context, function) == FVM_OK); /* [GC:] Host retains function until final unpin. */
    CHECK(fvm_rdispatch_create(0, &host.dispatch) == FVM_R_OK);
    CHECK(fvm_rdebug_create(context, host.dispatch, "/debug/ctl", service, &host, &host.debug, &host.resource) ==
          FVM_R_OK);
    for (i = 0; i < 2; ++i)
    {
        c = host.clients + i;
        CHECK(fvm_rdispatch_grant(host.dispatch,
                                  host.resource,
                                  11 + (unsigned long)i,
                                  FVM_R_READ | FVM_R_SUBSCRIBE | (i ? 0 : FVM_R_CONTROL)) == FVM_R_OK);
        CHECK(fvm_rfs_create(host.dispatch, 11 + (unsigned long)i, &c->fs) == FVM_R_OK);
        CHECK(fvm_r9p_create(c->fs, &c->server) == FVM_R_OK &&
              fvm_sbindings_init(&c->events, &c->event_slot, 1) == FVM_S_OK);
        put(p, FVM_R9P_MSIZE, 4);
        n = 4 + string(p + 4, "9P2000");
        CHECK(send(c, 100, 65535, p, n, 101));
        put(p, 0, 4);
        put(p + 4, 0xffffffffUL, 4);
        n = 8 + string(p + 8, "untrusted-root-name");
        n += string(p + n, "");
        CHECK(send(c, 104, 1, p, n, 105));
        CHECK(walk(c, 1, 0, 2) && open_file(c, 1, 0) && pending_read(c, 1, 0, 100 + (unsigned int)i));
    }
    CHECK(fvm_debug_set_breakpoint_handler(context, fvm_rdebug_breakpoint, host.debug) == FVM_OK);
    CHECK(fvm_debug_map_action(context, "<function>", 0, "ninep-debug.as", 23) == FVM_OK);
    CHECK(fvm_debug_set_breakpoint(context, "<function>", 0, 1) == FVM_OK);
    CHECK(fvm_function_invoke(context, function, 0, 0, 0, &result) == FVM_OK && result.number == 42 && !host.failed &&
          host.stage == 2);
    CHECK(reply(host.clients + 1, 111, &response) &&
          fvm_rdebug_frame_read(response.data, response.size, &frame) == FVM_R_OK);
    CHECK(frame.engine == FVM_CRASH_ENGINE_INTERPRETER && !strcmp(frame.function, "<function>"));
    CHECK(reply(host.clients, 112, &response));
    for (i = 0; i < 2; ++i)
    {
        CHECK(event(host.clients + i, 120 + (unsigned int)i, FVM_RDEBUG_RESUME_EVENT, &snapshot) &&
              snapshot.state == FVM_RDEBUG_RUNNING && snapshot.stop == 1);
    }
    CHECK(fvm_debug_set_breakpoint_handler(context, 0, 0) == FVM_OK);
    for (i = 0; i < 2; ++i)
    {
        CHECK(fvm_r9p_destroy(&host.clients[i].server) == FVM_R_OK);
        CHECK(fvm_rfs_destroy(&host.clients[i].fs) == FVM_R_OK);
    }
    CHECK(fvm_rdebug_destroy(&host.debug) == FVM_R_OK && fvm_rdispatch_destroy(&host.dispatch) == FVM_R_OK);
    CHECK(fvm_gc_unpin(context, function) == FVM_OK); /* [GC:] Match the only host pin. */
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    puts("PASS real AVM1 debugger over two 9P file connections: STOP, claim, frame, resume, result=42");
    return 0;
}
