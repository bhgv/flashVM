/* FlashVM: C embedding API: capability.
 * File: src/capi/capability_c.cpp.
 */

#include "graph_constants.h"
#include "function_rpc_constants.h"
#include "flashvm/function.h"
#include "flashvm/serialization.h"
#include "flashvm/gc.h"
#include "../core/object_root_scope.hpp"
#include "../core/exception_internal.hpp"
#include "../core/capability_internal.hpp"
#include "flashvm/capability.h"
#include "flashvm/worker.h"
#include "flashvm/debug.h"
#include "../core/fault_wire.hpp"
#include "../../ports/network_port.h"
#include "../../ports/thread_port.h"
#include "../../ports/channel_sync.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <new>
#include <climits>
#include "zlib.h"

/* Each connection carries one bounded request. Acquisitions on the authority
   are nonblocking: waiting belongs to the client, never to the service loop. */
#ifdef OS_PORTS
namespace
{
const size_t limit = 64UL * 1024UL * 1024UL;

/* Define Entry struct and its representation. */
struct Entry
{
    fvm_detail::string token; /* Random bearer identity for this export. */
    unsigned long kind; /* Resource or live-function capability kind. */
    void *handle; /* Retained native resource or pinned original function. */
    FVM_Context *owner; /* [GC:] Original function heap, borrowed until context detachment. */
    /* Ordinary resource entries have no VM owner. */
    Entry() : kind(0), handle(0), owner(0) {}
};

/* Define Peer struct and its representation. */
struct Peer
{
    fvm_detail::string key;
    FVM_Context *context;
};

/* Exclusive ownership handoff for original function heaps, synchronized by the broker. */
struct FunctionOwner
{
    FVM_Context *context; /* [GC:] Heap retained by its host until quiescence and detachment. */
    bool transfer; /* Whether serialization exports function references instead of closures. */
    bool serving; /* Whether the authority may enter this otherwise idle heap. */
    unsigned long active; /* Number of calls preventing context teardown. */
    /* Start in host ownership with no exported transfer policy. */
    FunctionOwner() : context(0), transfer(false), serving(false), active(0) {}
};

/* Define Remote struct and its representation. */
struct Remote
{
    fvm_detail::string uri, host, session, token;
    unsigned short port;
    unsigned long kind;
};

/* Define Lock struct and its representation. */
struct Lock
{
    FVM_PortSync *sync;
    /* Acquire the current value. */
    Lock(FVM_PortSync *s) : sync(s)
    {
        fvm_port_sync_lock(s);
    }
    /* Release the resources owned by this Lock instance. */
    ~Lock()
    {
        fvm_port_sync_unlock(sync);
    }
};

/* Compute and return hex. */
bool hex(char *out)
{
    unsigned char bytes[16];
    if (!fvm_port_secure_random(bytes, sizeof(bytes)))
        return false;
    const char *digits = "0123456789abcdef";
    for (size_t i = 0; i < 16; ++i)
    {
        out[i * 2] = digits[bytes[i] >> 4];
        out[i * 2 + 1] = digits[bytes[i] & 15];
    }
    out[32] = 0;
    return true;
}

/* Compute and return transfer. */
bool transfer(FVM_PortSocket *s, void *data, size_t size, bool send)
{
    unsigned char *p = (unsigned char *)data;
    while (size)
    {
        long n = send ? fvm_port_socket_send(s, p, size) : fvm_port_socket_recv(s, p, size);
        if (n <= 0 || (size_t)n > size)
            return false;
        p += n;
        size -= (size_t)n;
    }
    return true;
}

/* Implement put for this subsystem. */
void put(unsigned char *p, unsigned long n)
{

    /* Computation / Вычисление и обновление состояния. */
    p[0] = (unsigned char)(n >> 24);
    p[1] = (unsigned char)(n >> 16);
    p[2] = (unsigned char)(n >> 8);
    p[3] = (unsigned char)n;
}

/* Read the current value. */
unsigned long get(const unsigned char *p)
{
    return ((unsigned long)p[0] << 24) | ((unsigned long)p[1] << 16) | ((unsigned long)p[2] << 8) |
           p[3];
}

/* Compute and return valid hex. */
bool valid_hex(const fvm_detail::string &s)
{
    if (s.size() != 32)
        return false;
    for (size_t i = 0; i < s.size(); ++i)
        if (!((s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'f')))
            return false;
    return true;
}

/* Parse the current value. */
bool parse(const char *uri, Remote &r)
{
    if (!uri || std::strlen(uri) > 512 || std::strncmp(uri, "fvm://", 6))
        return false;
    const char *colon = std::strchr(uri + 6, ':');
    if (!colon || colon == uri + 6)
        return false;
    const char *slash = std::strchr(colon + 1, '/');
    if (!slash)
        return false;
    unsigned long port = 0;
    for (const char *p = colon + 1; p < slash; ++p)
    {
        if (*p < '0' || *p > '9')
            return false;
        port = port * 10 + *p - '0';
        if (port > 65535)
            return false;
    }
    if (!port)
        return false;
    const char *second = std::strchr(slash + 1, '/');
    if (!second)
        return false;
    const char *third = std::strchr(second + 1, '/');
    if (!third || !third[1] || third[2])
        return false;

    /* Computation / Вычисление и обновление состояния. */
    r.uri = uri;
    r.host = fvm_detail::string(uri + 6, (size_t)(colon - uri - 6));
    r.session = fvm_detail::string(slash + 1, (size_t)(second - slash - 1));
    r.token = fvm_detail::string(second + 1, (size_t)(third - second - 1));
    r.port = (unsigned short)port;
    r.kind = (unsigned long)(third[1] - '0');
    return valid_hex(r.session) && valid_hex(r.token) && (r.kind == 1 || r.kind == 8 || r.kind == FVM_FUNCTION_CAPABILITY_KIND);
}
} // namespace
/* Define FVM_ResourceBroker struct and its representation. */
struct FVM_ResourceBroker
{
    FVM_PortSync *sync;
    FVM_PortSocket *listener;
    FVM_PortThread *thread;
    unsigned long references;
    bool stopped;
    unsigned short port;
    fvm_detail::string host, session;
    fvm_detail::vector<Entry> entries;
    fvm_detail::vector<Peer> peers;
    fvm_detail::vector<FunctionOwner> function_owners; /* [GC:] Exclusive function service heaps. */
    FVM_Runtime *runtime;
    size_t adapter_contexts;        /* [GC:] Scoped graph-conversion contexts currently alive. */
    unsigned long adapter_requests; /* [GC:] Saturating graph-adapter creation counter. */
};
namespace
{
/* [GC:] A wire adapter owns only intermediate graph replicas, not client script execution state. */
struct AdapterContext
{
    FVM_Context *context; /* [GC:] Per-request graph conversion context, null before creation. */
    FVM_ResourceBroker
        *broker; /* [GC:] Borrowed authority used to export shared native capabilities. */

    /* [GC:] Start without publishing an adapter or claiming a broker reference. */
    AdapterContext() : context(0), broker(0)
    {
    }

    /* [GC:] Create transient graph storage without adding a persistent mutex ownership peer. */
    FVM_Result create(FVM_ResourceBroker *authority)
    {
        /* [GC:] Failure leaves no context or lifetime-counter mutation. */
        FVM_Result result = fvm_context_create(authority->runtime, &context);
        if (result != FVM_OK)
            return result;
        /* [GC:] The service owns the broker while this request runs; the VM pointer is borrowed. */
        broker = authority;
        context->broker = authority;
        Lock guard(authority->sync);
        /* [GC:] Publish adapter lifetime only after complete context initialization. */
        ++authority->adapter_contexts;
        /* [GC:] Saturating history never wraps on long-running services. */
        if (authority->adapter_requests < ULONG_MAX)
            ++authority->adapter_requests;
        return FVM_OK;
    }

    /* [GC:] Destroy invisible transport replicas without executing their user __gc handlers. */
    ~AdapterContext()
    {
        if (!context)
            return;
        /* [GC:] Graph conversion must not produce extra user finalizer effects in an intermediary
         * VM. */
        for (size_t i = 0; i < context->objects.size(); ++i)
            /* [GC:] Every intermediate shell skips user finalizer dispatch during adapter teardown.
             */
            context->objects[i]->finalized = true;
        /* [GC:] The borrowed broker must not be released by ordinary context teardown. */
        context->broker = 0;
        /* [GC:] Native payload references and all arena buffers are released on every dispatch
         * exit. */
        fvm_context_destroy(context);
        Lock guard(broker->sync);
        /* [GC:] No adapter remains live when the request's serialized reply is published. */
        --broker->adapter_contexts;
    }
};

/* Distinguish process identities even when context counters coincide on two
   machines. The nonce is initialized once under the registry lock. */
char client_nonce[33] = {0};

/* Compute and return nonce. */
bool nonce(char *out)
{
    fvm_port_registry_lock();
    bool ok = client_nonce[0] || hex(client_nonce);
    if (ok)
        std::memcpy(out, client_nonce, 33);
    fvm_port_registry_unlock();
    return ok;
}

/* Optional error records have a portable fixed layout defined by the fault codec. */
const size_t fault_record_size = FVM_FAULT_WIRE_SIZE;

/* Compute and return rpc. */
FVM_Result rpc(Remote *r, FVM_Context *c, int op, const unsigned char *input, size_t size,
               unsigned char **output, size_t *length)
{
    if (output)
        *output = 0;
    if (length)
        *length = 0;
    if (size > limit)
        return FVM_INVALID_ARGUMENT;
    unsigned char header[112];

    /* Computation / Вычисление и обновление состояния. */
    std::memset(header, 0, sizeof(header));
    std::memcpy(header, "FVMC", 4);
    header[4] = 1;
    header[5] = (unsigned char)op;
    header[6] = (unsigned char)r->kind;
    std::memcpy(header + 8, r->session.c_str(), 32);
    std::memcpy(header + 40, r->token.c_str(), 32);
    char id[33];
    if (!nonce(id))
        return FVM_ERROR;

    /* Computation / Вычисление и обновление состояния. */
    std::memcpy(header + 72, id, 32);
    put(header + 104, c ? c->identity : 0);
    put(header + 108, (unsigned long)size);
    FVM_PortSocket *socket = fvm_port_tcp_connect(r->host.c_str(), r->port);
    if (!socket)
    {
        fvm_resource_fault(c, FVM_RESOURCE_UNREACHABLE, 0, "network.connect", r->uri.c_str());
        fvm_debug_resource_error(c, 0, "network.connect", r->uri.c_str());
        return FVM_ERROR;
    }
    fvm_port_socket_timeout(socket, 2000);

    /* Initialization / Инициализация локального состояния. */
    unsigned char response[8];
    bool ok = transfer(socket, header, sizeof(header), true) &&
              (!size || transfer(socket, (void *)input, size, true)) &&
              transfer(socket, response, 8, false);
    FVM_Result result = FVM_ERROR;
    if (ok)
    {
        unsigned long status = get(response), n = get(response + 4);
        result = status == 0   ? FVM_OK
                 : status == 5 ? FVM_NOT_FOUND
                 : status == 2 ? FVM_OUT_OF_MEMORY
                               : FVM_ERROR;
        if (status >= FVM_RESOURCE_CLOSED && status <= FVM_RESOURCE_FAILURE)
            fvm_resource_fault(c, status, 0, "network.request", r->uri.c_str());
        if (n > limit || (!output && n))
            ok = false;
        else if (n)
        {
            *output = (unsigned char *)std::malloc(n);
            if (!*output)
            {
                result = FVM_OUT_OF_MEMORY;
                ok = false;
            }
            else if (!transfer(socket, *output, n, false))
                ok = false;
        }
        /* Optional metadata never changes the primary status or accepts partial/unverified fields.
         */
        if (ok && result == FVM_ERROR && status >= FVM_RESOURCE_CLOSED &&
            status <= FVM_RESOURCE_FAILURE)
            if (c && c->fault_pending)
                fvm_fault_wire_decode(&c->fault, output ? *output : 0, (size_t)n);
        if (ok && length)
            *length = (size_t)n;
    }
    fvm_port_socket_close(socket);
    if (!ok)
    {
        if (output)
        {
            std::free(*output);
            *output = 0;
        }
        result = FVM_ERROR;
    }
    if (result == FVM_ERROR)
    {
        fvm_resource_fault(c, ok ? FVM_RESOURCE_FAILURE : FVM_RESOURCE_DISCONNECTED, 0,
                           "network.request", r->uri.c_str());
        fvm_debug_resource_error(c, 0, "network.request", r->uri.c_str());
    }
    return result;
}

/* Compute and return mutex request. */
FVM_Result mutex_request(void *raw, FVM_Context *c, int op, int *out)
{

    /* Initialization / Инициализация локального состояния. */
    unsigned char *data = 0;
    size_t size = 0;
    FVM_Result r = rpc((Remote *)raw, c, op, 0, 0, &data, &size);
    if (r == FVM_OK)
    {
        if (size != 1)
            r = FVM_ERROR;
        else if (out)
            *out = data[0];
    }
    std::free(data);
    return r;
}

/* Compute and return channel request. */
FVM_Result channel_request(void *raw, FVM_Context *c, int op, const unsigned char *data,
                           size_t size, unsigned char **out, size_t *length, int blocking)
{
    FVM_Result r;
    do
    {
        int stop = 0;
        if (c && c->worker)
            fvm_worker_stop_requested(c->worker, &stop);
        if (stop)
            return FVM_ERROR;
        unsigned char *discard = 0;
        size_t discarded = 0;
        r = rpc((Remote *)raw, c, op, data, size, out ? out : &discard,
                length ? length : &discarded);
        std::free(discard);
        if (r == FVM_NOT_FOUND && blocking)
            fvm_port_thread_sleep_us(1000);
    } while (r == FVM_NOT_FOUND && blocking);
    return r;
}

/* Destroy remote. */
void destroy_remote(void *p)
{
    delete (Remote *)p;
}

#include "function_rpc.inc"

/* Compute and return closed. */
FVM_Result closed(const Entry &e, int *out)
{
    return e.kind == 8 ? fvm_mutex_is_closed((FVM_Mutex *)e.handle, out)
                       : fvm_channel_is_closed((FVM_Channel *)e.handle, out);
}

/* Preserve a structured nested resource cause instead of replacing it with broker failure. */
FVM_Result channel_result(FVM_Result result, FVM_Context *context, unsigned long &fault,
                          FVM_ExceptionInfo &details)
{
    if (result != FVM_OK && context && context->fault_pending &&
        context->fault.code >= FVM_RESOURCE_CLOSED && context->fault.code <= FVM_RESOURCE_FAILURE)
    {
        fault = context->fault.code;
        /* Copy before a scoped adapter destroys its heap and pending diagnostic storage. */
        details = context->fault;
    }
    return result;
}

/* Dispatch the current value. */
FVM_Result dispatch(FVM_ResourceBroker *b, const unsigned char *h, const unsigned char *input,
                    size_t size, unsigned char **output, size_t *length, unsigned long &fault,
                    FVM_ExceptionInfo &details)
{
    fault = FVM_RESOURCE_STALE;

    /* Initialization / Инициализация локального состояния. */
    Entry e;
    FVM_Context *c = 0;
    /* [GC:] Wire transformations use a scoped replica heap instead of persistent client state. */
    AdapterContext adapter;
    {
        Lock guard(b->sync);
        if (b->stopped || std::memcmp(h + 8, b->session.c_str(), 32))
            return FVM_ERROR;
        bool found = false;
        for (size_t i = 0; i < b->entries.size(); ++i)
            if (b->entries[i].kind == h[6] && !std::memcmp(h + 40, b->entries[i].token.c_str(), 32))
            {
                e = b->entries[i];
                found = true;
                break;
            }
        if (!found)
            return FVM_ERROR;
        /* [GC:] Only mutex ownership operations need a stable client-context identity. */
        if ((h[5] == 1 || h[5] == 2) && e.kind == 8)
        {
            if (!get(h + 104))
                return FVM_ERROR;
            char key[41];
            std::memcpy(key, h + 72, 32);
            std::sprintf(key + 32, "%08lx", get(h + 104));
            for (size_t i = 0; i < b->peers.size(); ++i)
                if (b->peers[i].key == key)
                    c = b->peers[i].context;
            if (!c)
            {
                Peer peer;
                peer.key = key;
                FVM_Result r = fvm_context_create(b->runtime, &peer.context);
                if (r != FVM_OK)
                    return r;
                peer.context->broker = b; // Borrowed; service owns all peer contexts.
                try
                {
                    b->peers.push_back(peer);
                }
                catch (...)
                {
                    peer.context->broker = 0;
                    fvm_context_destroy(peer.context);
                    throw;
                }
                c = peer.context;
            }
        }
    }
    fault = FVM_RESOURCE_FAILURE;
    /* [GC:] Receipt/inspection allocate graph state only for this authorized channel request. */
    if ((h[5] == 6 || h[5] == 9) && e.kind == 1)
    {
        /* [GC:] Adapter creation failure occurs before a queue packet can be consumed. */
        FVM_Result result = adapter.create(b);
        if (result != FVM_OK)
            return result;
        /* [GC:] The codec/export helper sees the borrowed authority for this scoped heap. */
        c = adapter.context;
    }
    if (c)
        fvm_exception_clear(c);

    /* Initialization / Инициализация локального состояния. */
    int state = 0;
    FVM_Result r = closed(e, &state);
    if (r != FVM_OK)
        return r;
    if (h[5] != 0 && h[5] != 8 && state)
    {
        fault = FVM_RESOURCE_CLOSED;
        return FVM_ERROR;
    }
    if (h[5] == 0 || (h[5] == 3 && e.kind == 8) || h[5] == 4 || (h[5] == 1 && e.kind == 8) ||
        (h[5] == 2 && e.kind == 8))
    {
        if (size)
            return FVM_INVALID_ARGUMENT;
        if (h[5] == 1)
            r = fvm_mutex_set(c, (FVM_Mutex *)e.handle, 0);
        else if (h[5] == 2)
            r = fvm_mutex_reset(c, (FVM_Mutex *)e.handle);
        else if (h[5] == 3)
            r = fvm_mutex_touch((FVM_Mutex *)e.handle, &state);
        else if (h[5] == 4)
        {
            if (e.kind == 8)
                r = fvm_mutex_close((FVM_Mutex *)e.handle);
            else
                fvm_channel_close((FVM_Channel *)e.handle);
            state = 1;
        }
        if (r != FVM_OK)
        {
            if (c && c->fault_pending)
            {
                fault = c->fault.code;
                details = c->fault;
            }
            return r;
        }
        *output = (unsigned char *)std::malloc(1);
        if (!*output)
            return FVM_OUT_OF_MEMORY;
        **output = (unsigned char)state;
        *length = 1;
        return FVM_OK;
    }
    if (e.kind != 1)
        return FVM_INVALID_ARGUMENT;
    /* Debug controls are nonblocking on the authority, including a frozen receive queue. */
    if (h[5] == 7)
    {
        if (size != 1 || input[0] > 1)
            return FVM_INVALID_ARGUMENT;
        return fvm_debug_channel_freeze((FVM_Channel *)e.handle, input[0]);
    }
    if (h[5] == 8)
    {
        if (size)
            return FVM_INVALID_ARGUMENT;
        FVM_DebugChannel snapshot;
        r = fvm_debug_channel_state((FVM_Channel *)e.handle, &snapshot);
        if (r != FVM_OK)
            return r;
        if (snapshot.pending > 4294967295UL || snapshot.capacity > 4294967295UL)
            return FVM_UNSUPPORTED;
        *output = (unsigned char *)std::malloc(12);
        if (!*output)
            return FVM_OUT_OF_MEMORY;
        put(*output, (unsigned long)snapshot.pending);
        put(*output + 4, (unsigned long)snapshot.capacity);
        put(*output + 8, (snapshot.closed ? 1UL : 0UL) | (snapshot.frozen ? 2UL : 0UL));
        *length = 12;
        return FVM_OK;
    }
    int stream = 0;
    fvm_channel_is_stream((FVM_Channel *)e.handle, &stream);
    /* Inspection bypasses read freezing, retains queue packets and restores file cursors. */
    if (h[5] == 9)
    {
        if (size != 4)
            return FVM_INVALID_ARGUMENT;
        size_t index = (size_t)get(input);
        r = stream ? fvm_channel_file_peek_record((FVM_Channel *)e.handle, index, output, length)
                   : fvm_channel_peek_wire(c, (FVM_Channel *)e.handle, index, output, length);
        return channel_result(r, c, fault, details);
    }
    if (h[5] == 5)
    {
        /* The VM graph decoder validates contents at receipt. Reject truncated
           records before enqueueing so a malformed peer cannot split framing. */
        if (size < 16 || (std::memcmp(input, "FVMG\0\0\0", 7) || (input[FVM_GRAPH_VERSION_OFFSET] < FVM_GRAPH_MIN_WIRE_VERSION || input[FVM_GRAPH_VERSION_OFFSET] > FVM_GRAPH_WIRE_VERSION)) ||
            get(input + 8) != size - 16 ||
            get(input + 12) != (unsigned long)crc32(0, input + 16, (uInt)(size - 16)))
            return FVM_ERROR;
        return stream ? fvm_channel_stream_send(c, (FVM_Channel *)e.handle, input, size)
                      : fvm_channel_send_wire((FVM_Channel *)e.handle, input, size);
    }
    if (h[5] == 6 && !size)
    {
        r = stream ? fvm_channel_stream_receive(c, (FVM_Channel *)e.handle, output, length, 0)
                   : fvm_channel_receive_wire(c, (FVM_Channel *)e.handle, output, length);
        return channel_result(r, c, fault, details);
    }
    return FVM_INVALID_ARGUMENT;
}

/* Implement serve for this subsystem. */
void serve(void *arg)
{
    FVM_ResourceBroker *b = (FVM_ResourceBroker *)arg;
    for (;;)
    {
        {
            Lock guard(b->sync);
            if (b->stopped)
                break;
        }
        FVM_PortSocket *s = fvm_port_tcp_accept(b->listener, 100);
        if (!s)
            continue;
        fvm_port_socket_timeout(s, 2000);
        unsigned char h[112], response[8], *input = 0, *output = 0;
        size_t length = 0;
        FVM_Result result = FVM_ERROR;
        unsigned long fault = FVM_RESOURCE_FAILURE;
        FVM_ExceptionInfo details;
        std::memset(&details, 0, sizeof(details));
        bool received = transfer(s, h, sizeof(h), false);
        if (received && !std::memcmp(h, "FVMC", 4) &&
            h[4] == FVM_FUNCTION_RPC_VERSION && h[6] == FVM_FUNCTION_CAPABILITY_KIND)
        {
            serve_function(b, s, h);
            fvm_port_socket_close(s);
            continue;
        }
        if (received && !std::memcmp(h, "FVMC", 4) && h[4] == 1 && get(h + 108) <= limit)
        {
            size_t size = (size_t)get(h + 108);
            input = size ? (unsigned char *)std::malloc(size) : 0;
            if ((!size || input) && (!size || transfer(s, input, size, false)))
            {
                try
                {
                    result = dispatch(b, h, input, size, &output, &length, fault, details);
                }
                catch (...)
                {
                    result = FVM_OUT_OF_MEMORY;
                }
            }
        }
        /* Optional details are best-effort; allocation failure preserves the original error code.
         */
        if (result != FVM_OK && result != FVM_NOT_FOUND && result != FVM_OUT_OF_MEMORY)
        {
            std::free(output);
            output = 0;
            length = 0;
            if (details.code == fault && details.kind[0] && details.resource <= 4294967295UL &&
                details.origin_thread <= 4294967295UL)
            {
                output = (unsigned char *)std::malloc(fault_record_size);
                if (output)
                {
                    fvm_fault_wire_encode(output, details);
                    length = fault_record_size;
                }
            }
        }
        put(response, result == FVM_OK              ? 0
                      : result == FVM_NOT_FOUND     ? 5
                      : result == FVM_OUT_OF_MEMORY ? 2
                                                    : fault);
        put(response + 4, (unsigned long)length);
        if (transfer(s, response, 8, true) && length)
            transfer(s, output, length, true);
        std::free(input);
        std::free(output);
        fvm_port_socket_close(s);
    }
}
} // namespace
/* Read the live-function transfer policy under the authority lock. */
bool fvm_function_transfer_enabled(FVM_Context *c)
{
    if (!c || !c->broker)
        return false;
    Lock guard(c->broker->sync);
    FunctionOwner *owner = function_owner(c->broker, c);
    return !c->broker->stopped && owner && owner->transfer;
}
/* Recognize proxies independently of transfer policy, including forwarded references. */
bool fvm_function_is_proxy(FVM_Object *function)
{
    return function && function->invocable.callback == function_proxy;
}
/* Create/reconfigure host-owned export policy before transferring heap ownership. */
extern "C" FVM_Result fvm_context_function_transfer(FVM_Context *c, int enabled)
{
    if (!c || !c->broker || (enabled != 0 && enabled != 1))
        return FVM_INVALID_ARGUMENT;
    try
    {
        Lock guard(c->broker->sync);
        if (c->broker->stopped)
            return FVM_ERROR;
        FunctionOwner *owner = function_owner(c->broker, c);
        if (owner && (owner->serving || owner->active))
            return FVM_ERROR;
        if (!owner)
        {
            FunctionOwner entry;
            entry.context = c;
            c->broker->function_owners.push_back(entry);
            owner = function_owner(c->broker, c);
        }
        owner->transfer = enabled != 0;
        return FVM_OK;
    }
    catch (...) { return FVM_OUT_OF_MEMORY; }
}
/* Transfer exclusive ownership to the broker or wait until all original-heap calls finish. */
extern "C" FVM_Result fvm_context_function_serve(FVM_Context *c, int enabled)
{
    if (!c || !c->broker || (enabled != 0 && enabled != 1))
        return FVM_INVALID_ARGUMENT;
    FVM_ResourceBroker *b = c->broker;
    Lock guard(b->sync);
    FunctionOwner *owner = function_owner(b, c);
    if (!owner || (b->stopped && enabled) || fvm_port_thread_is_current(b->thread))
        return FVM_ERROR;
    if (enabled)
    {
        /* An already handed-off heap may currently be executing on the service thread. */
        if (owner->serving)
            return FVM_OK;
        if (c->gc_frames || c->gc_native_depth || c->script_depth || c->destroying || c->gc_collecting ||
            c->exception_pending || c->fault_pending)
            return FVM_INVALID_ARGUMENT;
        owner->serving = true;
    }
    else
    {
        owner->serving = false;
        while ((owner = function_owner(b, c))->active)
            fvm_port_sync_wait(b->sync);
    }
    return FVM_OK;
}
/* [GC:] Reclaim an idle original heap before destruction or broker replacement. */
FVM_Result fvm_function_context_quiesce(FVM_Context *c)
{
    if (!c || !c->broker)
        return FVM_OK;
    FVM_ResourceBroker *b = c->broker;
    Lock guard(b->sync);
    FunctionOwner *owner = function_owner(b, c);
    if (!owner)
        return FVM_OK;
    if (owner->active && fvm_port_thread_is_current(b->thread))
        return FVM_ERROR;
    owner->serving = false;
    while ((owner = function_owner(b, c))->active)
        fvm_port_sync_wait(b->sync);
    return FVM_OK;
}
/* [GC:] Remove all exported roots on the original owner thread before its heap is released. */
void fvm_function_context_cleanup(FVM_Context *c)
{
    if (!c || !c->broker || fvm_function_context_quiesce(c) != FVM_OK)
        return;
    FVM_ResourceBroker *b = c->broker;
    Lock guard(b->sync);
    for (size_t i = b->entries.size(); i; --i)
        if (b->entries[i-1].kind == FVM_FUNCTION_CAPABILITY_KIND && b->entries[i-1].owner == c)
        {
            fvm_gc_unpin(c, (FVM_Object *)b->entries[i-1].handle);
            b->entries.erase(b->entries.begin() + (i-1));
        }
    for (size_t i = b->function_owners.size(); i; --i)
        if (b->function_owners[i-1].context == c)
            b->function_owners.erase(b->function_owners.begin() + (i-1));
}
/* Export a stable random target ID while retaining the exact original managed function. */
FVM_Result fvm_function_capability_export(FVM_Context *c, FVM_Object *function,
                                          fvm_detail::string &uri)
{
    if (!c || !function || function->owner != c)
        return FVM_INVALID_ARGUMENT;
    if (fvm_function_is_proxy(function))
    {
        FVM_Object *capture = function->invocable.root;
        if (!capture || capture->owner != c)
            return FVM_INVALID_ARGUMENT;
        fvm_detail::map<fvm_detail::string, FVM_Value>::iterator address = capture->properties.find("uri");
        if (address == capture->properties.end() || address->second.type != FVM_VALUE_STRING)
            return FVM_INVALID_ARGUMENT;
        uri = address->second.string;
        return FVM_OK;
    }
    if (!function->invocable.callback || !c->broker)
        return FVM_UNSUPPORTED;
    FVM_ResourceBroker *b = c->broker;
    Lock guard(b->sync);
    FunctionOwner *owner = function_owner(b, c);
    if (b->stopped || !owner || !owner->transfer)
        return FVM_UNSUPPORTED;
    fvm_detail::string token;
    for (size_t i = 0; i < b->entries.size(); ++i)
        if (b->entries[i].kind == FVM_FUNCTION_CAPABILITY_KIND && b->entries[i].owner == c &&
            b->entries[i].handle == function)
            token = b->entries[i].token;
    if (token.empty())
    {
        char id[FVM_FUNCTION_RPC_ID_STORAGE];
        if (!hex(id))
            return FVM_ERROR;
        Entry entry;
        entry.kind = FVM_FUNCTION_CAPABILITY_KIND;
        entry.owner = c;
        entry.handle = function;
        entry.token = id;
        token = entry.token;
        FVM_Result status = fvm_gc_pin(c, function);
        if (status != FVM_OK)
            return status;
        try { b->entries.push_back(entry); }
        catch (...) { fvm_gc_unpin(c, function); throw; }
    }
    char address[FVM_FUNCTION_RPC_URI_CAPACITY];
    std::sprintf(address, "fvm://%s:%u/%s/%s/%lu", b->host.c_str(), (unsigned)b->port,
                 b->session.c_str(), token.c_str(), FVM_FUNCTION_CAPABILITY_KIND);
    uri = address;
    return FVM_OK;
}
/* [GC:] Build a managed callable facade; no foreign code or userdata pointer is copied. */
FVM_Result fvm_function_capability_import(FVM_Context *c, const char *uri, FVM_Object **out)
{
    if (!c || !out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    Remote parsed;
    if (!parse(uri, parsed) || parsed.kind != FVM_FUNCTION_CAPABILITY_KIND)
        return FVM_INVALID_ARGUMENT;
    FVM_Object *capture = 0;
    FVM_Result status = fvm_object_create(c, &capture);
    if (status != FVM_OK)
        return status;
    FVM_ObjectRootScope roots(c, capture);
    FVM_Value address;
    fvm_value_string(&address, uri);
    status = fvm_object_set(c, capture, "uri", &address);
    return status == FVM_OK ? fvm_function_create_native(c, function_proxy, capture, capture, out) : status;
}

/* Compute and return broker start. */
extern "C" FVM_Result fvm_broker_start(const char *bind, const char *host, unsigned short port,
                                       FVM_ResourceBroker **out)
{
    if (!bind || !host || !out || !host[0] || std::strlen(host) > 200 || std::strchr(host, ':') ||
        std::strchr(host, '/'))
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    FVM_ResourceBroker *b = new (std::nothrow) FVM_ResourceBroker;
    if (!b)
        return FVM_OUT_OF_MEMORY;

    /* Computation / Вычисление и обновление состояния. */
    b->sync = 0;
    b->listener = 0;
    b->thread = 0;
    b->runtime = 0;
    b->references = 1;
    b->stopped = false;
    /* [GC:] Broker wire conversion starts without active scoped adapters. */
    b->adapter_contexts = 0;
    /* [GC:] Count successful adapter creation independently of persistent mutex peers. */
    b->adapter_requests = 0;
    try
    {
        char id[FVM_FUNCTION_RPC_ID_STORAGE];
        if (!hex(id))
            throw FVM_ERROR;
        b->host = host;
        b->session = id;
        b->sync = fvm_port_sync_create();
        if (!b->sync)
            throw FVM_OUT_OF_MEMORY;
        FVM_Config config;
        fvm_config_init(&config);
        config.features = 0;
        FVM_Result r = fvm_runtime_create(&config, &b->runtime);
        if (r != FVM_OK)
            throw r;
        b->listener = fvm_port_tcp_listen(bind, port, &b->port);
        if (!b->listener)
            throw FVM_ERROR;
        b->thread = fvm_port_thread_start(serve, b);
        if (!b->thread)
            throw FVM_ERROR;
        *out = b;
        return FVM_OK;
    }
    catch (...)
    {
        if (b->listener)
            fvm_port_socket_close(b->listener);
        if (b->runtime)
            fvm_runtime_destroy(b->runtime);
        if (b->sync)
            fvm_port_sync_destroy(b->sync);
        delete b;
        return FVM_ERROR;
    }
}

/* Compute and return broker port. */
extern "C" unsigned short fvm_broker_port(FVM_ResourceBroker *b)
{
    return b ? b->port : 0;
}
/* [GC:] Copy synchronized broker lifetime counters without observing mutable VM heaps. */
extern "C" FVM_Result fvm_broker_stats(FVM_ResourceBroker *b, FVM_BrokerStats *out)
{
    if (!b || !out)
        return FVM_INVALID_ARGUMENT;
    Lock guard(b->sync);
    /* Stop clears registries outside this lock only after setting stopped under it. */
    if (b->stopped)
        return FVM_ERROR;
    FVM_BrokerStats snapshot;
    snapshot.peer_contexts = b->peers.size();
    snapshot.exported_resources = b->entries.size();
    /* [GC:] Read only scalar counters maintained under the same broker lock. */
    snapshot.adapter_contexts = b->adapter_contexts;
    /* [GC:] The historical creation counter may saturate but never wraps. */
    snapshot.adapter_requests = b->adapter_requests;
    *out = snapshot;
    return FVM_OK;
}
/* Retain broker. */
extern "C" FVM_Result fvm_broker_retain(FVM_ResourceBroker *b)
{
    if (!b)
        return FVM_INVALID_ARGUMENT;
    Lock guard(b->sync);
    if (b->references == ULONG_MAX)
        return FVM_ERROR;
    ++b->references;
    return FVM_OK;
}

/* Implement broker stop for this subsystem. */
extern "C" void fvm_broker_stop(FVM_ResourceBroker *b)
{
    if (!b || fvm_port_thread_is_current(b->thread))
        return;
    {
        Lock guard(b->sync);
        if (b->stopped)
            return;
        b->stopped = true;
    }

    /* Computation / Вычисление и обновление состояния. */
    fvm_port_thread_join(b->thread);
    b->thread = 0;
    fvm_port_socket_close(b->listener);
    b->listener = 0;
    for (size_t i = 0; i < b->peers.size(); ++i)
    {
        b->peers[i].context->broker = 0;
        fvm_context_destroy(b->peers[i].context);
    }
    b->peers.clear();
    Lock entries_guard(b->sync);
    for (size_t i = 0; i < b->entries.size(); ++i)
    {
        Entry &e = b->entries[i];
        if (e.kind == 8)
            fvm_mutex_release((FVM_Mutex *)e.handle);
        else if (e.kind != FVM_FUNCTION_CAPABILITY_KIND)
            fvm_channel_release((FVM_Channel *)e.handle);
    }
    /* Function entries retain only owner-managed roots; detach them on the owner thread. */
    for (size_t i = b->entries.size(); i; --i)
        if (b->entries[i-1].kind != FVM_FUNCTION_CAPABILITY_KIND)
            b->entries.erase(b->entries.begin() + (i-1));
}

/* Release broker. */
extern "C" void fvm_broker_release(FVM_ResourceBroker *b)
{
    if (!b)
        return;
    bool last;
    {
        Lock guard(b->sync);
        last = --b->references == 0;
    }
    if (!last)
        return;
    fvm_broker_stop(b);

    /* Cleanup / Освобождение ресурсов. */
    fvm_runtime_destroy(b->runtime);
    fvm_port_sync_destroy(b->sync);
    delete b;
}

/* Compute and return capability export. */
FVM_Result fvm_capability_export(FVM_Context *c, FVM_Object *o, fvm_detail::string &uri)
{
    if (o && (fvm_function_is_proxy(o) || (fvm_function_transfer_enabled(c) &&
        (o->resource_kind == FVM_RESOURCE_NATIVE_FUNCTION || o->resource_kind == FVM_RESOURCE_SCRIPT_FUNCTION))))
        return fvm_function_capability_export(c, o, uri);
    if (!c || c->destroying || !o || o->owner != c || !o->resource ||
        (o->resource_kind != FVM_RESOURCE_CHANNEL && o->resource_kind != FVM_RESOURCE_MUTEX))
        return FVM_INVALID_ARGUMENT;
    Entry entry;

    /* Computation / Вычисление и обновление состояния. */
    entry.kind = (unsigned long)o->resource_kind;
    entry.handle = o->resource;

    /* Initialization / Инициализация локального состояния. */
    int state = 0;
    FVM_Result r = closed(entry, &state);
    if (r != FVM_OK || state)
    {
        fvm_resource_fault(c, state ? FVM_RESOURCE_CLOSED : FVM_RESOURCE_DISCONNECTED, 0,
                           "capability.export", "");
        fvm_resource_fault_resource(c, entry.kind, entry.handle, o->resource_origin);
        fvm_debug_resource_error(c, 0, "capability.export", "");
        return FVM_ERROR;
    }
    const char *address = entry.kind == 8 ? fvm_mutex_remote_address((FVM_Mutex *)entry.handle)
                                          : fvm_channel_remote_address((FVM_Channel *)entry.handle);
    if (address)
    {
        uri = address;
        return FVM_OK;
    }
    FVM_ResourceBroker *b = c->broker;
    if (!b)
        return FVM_UNSUPPORTED;
    Lock guard(b->sync);
    if (b->stopped)
        return FVM_ERROR;
    for (size_t i = 0; i < b->entries.size(); ++i)
        if (b->entries[i].kind == entry.kind && b->entries[i].handle == entry.handle)
            entry.token = b->entries[i].token;
    if (entry.token.empty())
    {
        char token[33];
        if (!hex(token))
            return FVM_ERROR;
        entry.token = token;
        r = entry.kind == 8 ? fvm_mutex_retain((FVM_Mutex *)entry.handle)
                            : fvm_channel_retain((FVM_Channel *)entry.handle);
        if (r != FVM_OK)
            return r;
        try
        {
            b->entries.push_back(entry);
        }
        catch (...)
        {
            if (entry.kind == 8)
                fvm_mutex_release((FVM_Mutex *)entry.handle);
            else
                fvm_channel_release((FVM_Channel *)entry.handle);
            throw;
        }
    }
    char uri_buffer[512];

    /* Computation / Вычисление и обновление состояния. */
    std::sprintf(uri_buffer, "fvm://%s:%u/%s/%s/%lu", b->host.c_str(), (unsigned int)b->port,
                 b->session.c_str(), entry.token.c_str(), entry.kind);
    uri = uri_buffer;
    return FVM_OK;
}

/* Compute and return capability import. */
FVM_Result fvm_capability_import(FVM_Context *c, const char *uri, unsigned long kind,
                                 FVM_Object **out)
{
    if (!c || c->destroying || !out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    Remote parsed;
    if (!parse(uri, parsed) || (kind && kind != parsed.kind))
        return FVM_INVALID_ARGUMENT;
    if (parsed.kind == FVM_FUNCTION_CAPABILITY_KIND)
        return fvm_function_capability_import(c, uri, out);
    if (c->broker && c->broker->session == parsed.session)
    {
        Lock guard(c->broker->sync);
        if (c->broker->stopped)
        {
            fvm_resource_fault(c, FVM_RESOURCE_STALE, 0, "capability.import", uri);
            fvm_resource_fault_resource(c, parsed.kind, 0, 0);
            fvm_debug_resource_error(c, 0, "capability.import", uri);
            return FVM_ERROR;
        }
        for (size_t i = 0; i < c->broker->entries.size(); ++i)
        {
            const Entry &e = c->broker->entries[i];
            if (e.kind == parsed.kind && e.token == parsed.token)
            {
                int state = 0;
                FVM_Result r = closed(e, &state);
                if (r != FVM_OK || state)
                {
                    fvm_resource_fault(c, FVM_RESOURCE_CLOSED, 0, "capability.import", uri);
                    fvm_resource_fault_resource(c, e.kind, e.handle, 0);
                    fvm_debug_resource_error(c, 0, "capability.import", uri);
                    return FVM_ERROR;
                }
                return e.kind == 8 ? fvm_extensions_wrap_mutex(c, (FVM_Mutex *)e.handle, out)
                                   : fvm_extensions_wrap_channel(c, (FVM_Channel *)e.handle, out);
            }
        }
        fvm_resource_fault(c, FVM_RESOURCE_STALE, 0, "capability.import", uri);
        fvm_resource_fault_resource(c, parsed.kind, 0, 0);
        return FVM_ERROR;
    }

    /* Initialization / Инициализация локального состояния. */
    unsigned char *data = 0;
    size_t length = 0;
    FVM_Result r = rpc(&parsed, c, 0, 0, 0, &data, &length);
    if (r == FVM_OK && (length != 1 || data[0]))
    {
        fvm_resource_fault(c, length == 1 ? FVM_RESOURCE_CLOSED : FVM_RESOURCE_FAILURE, 0,
                           "capability.import", uri);
        r = FVM_ERROR;
    }
    std::free(data);
    if (r != FVM_OK)
    {
        fvm_resource_fault_resource(c, parsed.kind, 0, 0);
        return r;
    }
    /* Repeated imports in an interpreter preserve proxy wrapper identity. */
    for (size_t i = 0; i < c->objects.size(); ++i)
    {
        FVM_Object *o = c->objects[i];
        if ((unsigned long)o->resource_kind != parsed.kind || !o->resource)
            continue;
        const char *address = parsed.kind == 8
                                  ? fvm_mutex_remote_address((FVM_Mutex *)o->resource)
                                  : fvm_channel_remote_address((FVM_Channel *)o->resource);
        if (address && !std::strcmp(address, uri))
        {
            *out = o;
            return FVM_OK;
        }
    }
    Remote *remote = new (std::nothrow) Remote;
    if (!remote)
        return FVM_OUT_OF_MEMORY;
    try
    {
        *remote = parsed;
    }
    catch (...)
    {
        delete remote;
        throw;
    }
    if (parsed.kind == 8)
    {
        FVM_Mutex *m = 0;
        r = fvm_mutex_create_remote(c, remote, mutex_request, destroy_remote, remote->uri.c_str(),
                                    &m);
        if (r != FVM_OK)
        {
            delete remote;
            return r;
        }
        r = fvm_extensions_wrap_mutex(c, m, out);
        fvm_mutex_release(m);
    }
    else
    {
        FVM_Channel *ch = 0;
        r = fvm_channel_create_remote(remote, channel_request, destroy_remote, remote->uri.c_str(),
                                      &ch);
        if (r != FVM_OK)
        {
            delete remote;
            return r;
        }
        r = fvm_extensions_wrap_channel(c, ch, out);
        fvm_channel_release(ch);
    }
    return r;
}
#else
/* Function export is disabled without platform sockets/threads. */
bool fvm_function_transfer_enabled(FVM_Context *) { return false; }
/* No callable proxy can be created by this configuration. */
bool fvm_function_is_proxy(FVM_Object *) { return false; }
/* Report unavailable function authority export. */
FVM_Result fvm_function_capability_export(FVM_Context *, FVM_Object *, fvm_detail::string &)
{ return FVM_UNSUPPORTED; }
/* Report unavailable function authority import. */
FVM_Result fvm_function_capability_import(FVM_Context *, const char *, FVM_Object **)
{ return FVM_UNSUPPORTED; }
/* No service thread can hold the source heap in this configuration. */
FVM_Result fvm_function_context_quiesce(FVM_Context *) { return FVM_OK; }
/* [GC:] No function export roots exist without platform transport. */
void fvm_function_context_cleanup(FVM_Context *) {}
/* Report unavailable live function transfer policy. */
extern "C" FVM_Result fvm_context_function_transfer(FVM_Context *, int)
{ return FVM_UNSUPPORTED; }
/* Report unavailable original-heap service ownership. */
extern "C" FVM_Result fvm_context_function_serve(FVM_Context *, int)
{ return FVM_UNSUPPORTED; }
/* Define FVM_ResourceBroker struct and its representation. */
struct FVM_ResourceBroker
{
};

/* Compute and return broker start. */
extern "C" FVM_Result fvm_broker_start(const char *, const char *, unsigned short,
                                       FVM_ResourceBroker **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    return FVM_UNSUPPORTED;
}

/* [GC:] Validate diagnostics arguments when the build has no network broker adapter. */
extern "C" FVM_Result fvm_broker_stats(FVM_ResourceBroker *b, FVM_BrokerStats *out)
{
    return !b || !out ? FVM_INVALID_ARGUMENT : FVM_UNSUPPORTED;
}
/* Compute and return broker port. */
extern "C" unsigned short fvm_broker_port(FVM_ResourceBroker *)
{
    return 0;
}
/* Retain broker. */
extern "C" FVM_Result fvm_broker_retain(FVM_ResourceBroker *)
{
    return FVM_UNSUPPORTED;
}

/* Implement broker stop for this subsystem. */
extern "C" void fvm_broker_stop(FVM_ResourceBroker *)
{
}

/* Release broker. */
extern "C" void fvm_broker_release(FVM_ResourceBroker *)
{
}

/* Compute and return capability export. */
FVM_Result fvm_capability_export(FVM_Context *, FVM_Object *, fvm_detail::string &)
{
    return FVM_UNSUPPORTED;
}

/* Compute and return capability import. */
FVM_Result fvm_capability_import(FVM_Context *, const char *, unsigned long, FVM_Object **)
{
    return FVM_UNSUPPORTED;
}
#endif
/* Implement capability context cleanup for this subsystem. */
void fvm_capability_context_cleanup(FVM_Context *c)
{
    fvm_function_context_cleanup(c);
    if (c->broker)
    {
        fvm_broker_release(c->broker);
        c->broker = 0;
    }
}

/* Update context broker. */
extern "C" FVM_Result fvm_context_set_broker(FVM_Context *c, FVM_ResourceBroker *b)
{
    if (fvm_function_context_quiesce(c) != FVM_OK)
        return FVM_ERROR;
    if (!c || c->destroying)
        return FVM_INVALID_ARGUMENT;
    if (b)
    {
        FVM_Result r = fvm_broker_retain(b);
        if (r != FVM_OK)
            return r;
    }
    fvm_capability_context_cleanup(c);
    c->broker = b;
    return FVM_OK;
}

/* Compute and return broker export. */
extern "C" FVM_Result fvm_broker_export(FVM_Context *c, FVM_Object *o, char **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    try
    {
        fvm_detail::string uri;
        FVM_Result r = fvm_capability_export(c, o, uri);
        if (r != FVM_OK)
            return r;
        *out = (char *)std::malloc(uri.size() + 1);
        if (!*out)
            return FVM_OUT_OF_MEMORY;
        std::memcpy(*out, uri.c_str(), uri.size() + 1);
        return FVM_OK;
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* Compute and return broker import. */
extern "C" FVM_Result fvm_broker_import(FVM_Context *c, const char *uri, FVM_Object **out)
{
    try
    {
        return fvm_capability_import(c, uri, 0, out);
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* Open channel network. */
extern "C" FVM_Result fvm_channel_open_network(FVM_Context *c, const char *uri, FVM_Channel **out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    FVM_Object *o = 0;
    try
    {
        FVM_Result r = fvm_capability_import(c, uri, 1, &o);
        if (r != FVM_OK)
            return r;
        r = fvm_channel_retain((FVM_Channel *)o->resource);
        if (r == FVM_OK)
            *out = (FVM_Channel *)o->resource;
        return r;
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
