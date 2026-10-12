/* FlashVM: C embedding API: capability.
 * File: src/capi/capability_c.cpp.
 */

#include "../core/exception_internal.hpp"
#include "../core/capability_internal.hpp"
#include "flashvm/capability.h"
#include "flashvm/worker.h"
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
    fvm_detail::string token;
    unsigned long kind;
    void *handle;
};

/* Define Peer struct and its representation. */
struct Peer
{
    fvm_detail::string key;
    FVM_Context *context;
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
    return valid_hex(r.session) && valid_hex(r.token) && (r.kind == 1 || r.kind == 8);
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
    FVM_Runtime *runtime;
};
namespace
{
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

/* Compute and return closed. */
FVM_Result closed(const Entry &e, int *out)
{
    return e.kind == 8 ? fvm_mutex_is_closed((FVM_Mutex *)e.handle, out)
                       : fvm_channel_is_closed((FVM_Channel *)e.handle, out);
}

/* Dispatch the current value. */
FVM_Result dispatch(FVM_ResourceBroker *b, const unsigned char *h, const unsigned char *input,
                    size_t size, unsigned char **output, size_t *length, unsigned long &fault)
{
    fault = FVM_RESOURCE_STALE;

    /* Initialization / Инициализация локального состояния. */
    Entry e;
    FVM_Context *c = 0;
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
        if (h[5] == 1 || h[5] == 2 || h[5] == 6)
        {
            if (!get(h + 104) && h[5] != 6)
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
    if (c)
        fvm_exception_clear(c);

    /* Initialization / Инициализация локального состояния. */
    int state = 0;
    FVM_Result r = closed(e, &state);
    if (r != FVM_OK)
        return r;
    if (h[5] != 0 && state)
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
                fault = c->fault.code;
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
    int stream = 0;
    fvm_channel_is_stream((FVM_Channel *)e.handle, &stream);
    if (h[5] == 5)
    {
        /* The VM graph decoder validates contents at receipt. Reject truncated
           records before enqueueing so a malformed peer cannot split framing. */
        if (size < 16 || (std::memcmp(input, "FVMG\0\0\0", 7) || (input[7] < 1 || input[7] > 7)) ||
            get(input + 8) != size - 16 ||
            get(input + 12) != (unsigned long)crc32(0, input + 16, (uInt)(size - 16)))
            return FVM_ERROR;
        return stream ? fvm_channel_stream_send(c, (FVM_Channel *)e.handle, input, size)
                      : fvm_channel_send_wire((FVM_Channel *)e.handle, input, size);
    }
    if (h[5] == 6 && !size)
        return stream ? fvm_channel_stream_receive(c, (FVM_Channel *)e.handle, output, length, 0)
                      : fvm_channel_receive_wire(c, (FVM_Channel *)e.handle, output, length);
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
        if (transfer(s, h, sizeof(h), false) && !std::memcmp(h, "FVMC", 4) && h[4] == 1 &&
            get(h + 108) <= limit)
        {
            size_t size = (size_t)get(h + 108);
            input = size ? (unsigned char *)std::malloc(size) : 0;
            if ((!size || input) && (!size || transfer(s, input, size, false)))
            {
                try
                {
                    result = dispatch(b, h, input, size, &output, &length, fault);
                }
                catch (...)
                {
                    result = FVM_OUT_OF_MEMORY;
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
    try
    {
        char id[33];
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
    if (!b)
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
    for (size_t i = 0; i < b->entries.size(); ++i)
    {
        Entry &e = b->entries[i];
        if (e.kind == 8)
            fvm_mutex_release((FVM_Mutex *)e.handle);
        else
            fvm_channel_release((FVM_Channel *)e.handle);
    }
    b->entries.clear();
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
    if (!c || c->destroying || !o || o->owner != c || !o->resource ||
        (o->resource_kind != 1 && o->resource_kind != 8))
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
    if (c->broker)
    {
        fvm_broker_release(c->broker);
        c->broker = 0;
    }
}

/* Update context broker. */
extern "C" FVM_Result fvm_context_set_broker(FVM_Context *c, FVM_ResourceBroker *b)
{
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
