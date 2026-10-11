#include "../core/runtime.hpp"
#include "../core/capability_internal.hpp"
#include "flashvm/mutex.h"
#include "flashvm/debug.h"
#include "flashvm/worker.h"
#include "../../ports/channel_sync.h"
#include "../../ports/thread_port.h"
#include <cstring>
#include <climits>
#include <new>

struct MutexWait
{
    unsigned long context, line, offset;
    char source[256], function[256];
    MutexWait *next;
};
struct FVM_Mutex
{
    unsigned long identity, origin, owner, references;
    bool closed;
    FVM_PortSync *remote_sync;
    void *remote;
    FVM_RemoteMutexFn request;
    void (*release_remote)(void *);
    const char *remote_address;
    unsigned long owner_line, owner_offset;
    char owner_source[256], owner_function[256];
    MutexWait *waiters;
    FVM_Mutex *next;
};
namespace
{
FVM_Mutex *mutexes = 0;
unsigned long next_identity = 0;
struct RegistryGuard
{
    RegistryGuard()
    {
        fvm_port_registry_lock();
    }
    ~RegistryGuard()
    {
        fvm_port_registry_unlock();
    }
};
/* Called only while holding the registry lock. Ownership itself retains the
   native mutex, so collecting its last wrapper cannot unlock a critical section. */
void drop(FVM_Mutex *m)
{
    if (--m->references)
        return;
    FVM_Mutex **link = &mutexes;
    while (*link != m)
        link = &(*link)->next;
    *link = m->next;
    if (m->release_remote)
        m->release_remote(m->remote);
    if (m->remote_sync)
        fvm_port_sync_destroy(m->remote_sync);
    delete m;
}
unsigned long waiting_for(unsigned long context)
{
    for (FVM_Mutex *m = mutexes; m; m = m->next)
        for (MutexWait *w = m->waiters; w; w = w->next)
            if (w->context == context)
                return m->owner;
    return 0;
}
bool cycle(unsigned long context, size_t bound)
{
    unsigned long current = waiting_for(context);
    for (size_t i = 0; current && i <= bound; ++i)
    {
        if (current == context)
            return true;
        current = waiting_for(current);
    }
    return false;
}
void row(FVM_DebugMutex &out, FVM_Mutex *m, MutexWait *w, size_t bound)
{
    std::memset(&out, 0, sizeof(out));
    out.identity = m->identity;
    out.origin = m->origin;
    out.owner = m->owner;
    out.state = m->owner != 0;
    if (m->owner)
    {
        out.owner_line = m->owner_line;
        out.owner_offset = m->owner_offset;
        std::memcpy(out.owner_source, m->owner_source, sizeof(out.owner_source));
        std::memcpy(out.owner_function, m->owner_function, sizeof(out.owner_function));
    }
    if (w)
    {
        out.waiter = w->context;
        out.line = w->line;
        out.offset = w->offset;
        std::memcpy(out.source, w->source, sizeof(out.source));
        std::memcpy(out.function, w->function, sizeof(out.function));
        out.deadlocked = cycle(w->context, bound);
    }
}
} // namespace
extern "C" unsigned long fvm_sync_next_identity()
{
    RegistryGuard guard;
    return next_identity == 4294967295UL ? 0 : ++next_identity;
}
extern "C" unsigned long fvm_context_identity(FVM_Context *c)
{
    return c ? c->identity : 0;
}
extern "C" FVM_Result fvm_mutex_create(FVM_Context *c, FVM_Mutex **out)
{
    if (!c || !out || c->destroying)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    FVM_Mutex *m = new (std::nothrow) FVM_Mutex;
    if (!m)
        return FVM_OUT_OF_MEMORY;
    RegistryGuard guard;
    if (next_identity == 4294967295UL)
    {
        delete m;
        return FVM_ERROR;
    }
    m->remote_sync = 0;
    m->closed = false;
    m->remote = 0;
    m->request = 0;
    m->release_remote = 0;
    m->remote_address = 0;
    m->identity = ++next_identity;
    m->origin = c->identity;
    m->owner = 0;
    m->references = 1;
    m->waiters = 0;
    m->next = mutexes;
    mutexes = m;
    *out = m;
    return FVM_OK;
}
extern "C" FVM_Result fvm_mutex_retain(FVM_Mutex *m)
{
    if (!m)
        return FVM_INVALID_ARGUMENT;
    RegistryGuard guard;
    if (m->references == ULONG_MAX)
        return FVM_ERROR;
    ++m->references;
    return FVM_OK;
}
extern "C" void fvm_mutex_release(FVM_Mutex *m)
{
    if (m)
    {
        RegistryGuard guard;
        drop(m);
    }
}
extern "C" unsigned long fvm_mutex_origin(FVM_Mutex *m)
{
    return m ? m->origin : 0;
}
extern "C" unsigned long fvm_mutex_identity(FVM_Mutex *m)
{
    return m ? m->identity : 0;
}
static FVM_Result touch_impl(FVM_Mutex *m, int *out)
{
    if (!m || !out)
        return FVM_INVALID_ARGUMENT;
    RegistryGuard guard;
    if (m->closed)
        return FVM_ERROR;
    *out = m->owner != 0;
    return FVM_OK;
}
static FVM_Result reset_impl(FVM_Context *c, FVM_Mutex *m)
{
    if (!c || !m || c->destroying)
        return FVM_INVALID_ARGUMENT;
    RegistryGuard guard;
    if (m->closed || m->owner != c->identity)
        return FVM_ERROR;
    m->owner = 0;
    drop(m);
    return FVM_OK;
}
static FVM_Result set_impl(FVM_Context *c, FVM_Mutex *m, int blocking)
{
    if (!c || !m || c->destroying)
        return FVM_INVALID_ARGUMENT;
    MutexWait wait;
    std::memset(&wait, 0, sizeof(wait));
    wait.context = c->identity;
    wait.line = c->debug_line;
    wait.offset = c->debug_offset;
    std::strncpy(wait.source, c->debug_source.c_str(), sizeof(wait.source) - 1);
    std::strncpy(wait.function, c->debug_function.c_str(), sizeof(wait.function) - 1);
    fvm_port_registry_lock();
    if (m->closed || m->owner == c->identity)
    {
        fvm_port_registry_unlock();
        return FVM_ERROR;
    }
    if (!blocking && m->owner)
    {
        fvm_port_registry_unlock();
        return FVM_NOT_FOUND;
    }
    if (m->references > ULONG_MAX - 2)
    {
        fvm_port_registry_unlock();
        return FVM_ERROR;
    }
    ++m->references; // The waiting operation owns a reference until it returns.
    wait.next = m->waiters;
    m->waiters = &wait;
    FVM_Result result = FVM_OK;
    while (m->owner && !m->closed)
    {
        fvm_port_registry_unlock();
        int stop = 0;
        if (c->worker)
            fvm_worker_stop_requested(c->worker, &stop);
        if (!stop)
            fvm_port_thread_sleep_us(1000);
        fvm_port_registry_lock();
        if (stop)
        {
            result = FVM_ERROR;
            break;
        }
    }
    if (m->closed)
        result = FVM_ERROR;
    MutexWait **link = &m->waiters;
    while (*link != &wait)
        link = &(*link)->next;
    *link = wait.next;
    if (result == FVM_OK)
    {
        m->owner = c->identity;
        m->owner_line = c->debug_line;
        m->owner_offset = c->debug_offset;
        std::memset(m->owner_source, 0, sizeof(m->owner_source));
        std::memset(m->owner_function, 0, sizeof(m->owner_function));
        std::strncpy(m->owner_function, c->debug_function.c_str(), sizeof(m->owner_function) - 1);
        std::strncpy(m->owner_source, c->debug_source.c_str(), sizeof(m->owner_source) - 1);
        ++m->references;
    }
    drop(m);
    fvm_port_registry_unlock();
    return result;
}
void fvm_mutex_context_cleanup(FVM_Context *c)
{
    /* Network cleanup must never run while holding the process registry lock. */
    for (;;)
    {
        fvm_port_registry_lock();
        FVM_Mutex *found = 0;
        for (FVM_Mutex *m = mutexes; m; m = m->next)
            if (m->owner == c->identity)
            {
                found = m;
                break;
            }
        if (!found)
        {
            fvm_port_registry_unlock();
            break;
        }
        if (found->references == ULONG_MAX)
        {
            fvm_port_registry_unlock();
            break;
        }
        ++found->references; // Keep the operation alive even if close drops ownership.
        fvm_port_registry_unlock();
        if (found->remote_sync)
            fvm_port_sync_lock(found->remote_sync);
        {
            RegistryGuard guard;
            if (found->owner != c->identity)
            {
                if (found->remote_sync)
                    fvm_port_sync_unlock(found->remote_sync);
                drop(found);
                continue;
            }
            found->owner = 0;
        }
        if (found->request)
        {
            int ignored = 0;
            if (found->request(found->remote, c, 2, &ignored) != FVM_OK)
                fvm_debug_resource_error(c, found->identity, "mutex.cleanup",
                                         found->remote_address);
        }
        if (found->remote_sync)
            fvm_port_sync_unlock(found->remote_sync);
        fvm_port_registry_lock();
        drop(found); // Ownership reference.
        drop(found); // Cleanup operation reference.
        fvm_port_registry_unlock();
    }
}
FVM_Result fvm_mutex_create_remote(FVM_Context *c, void *remote, FVM_RemoteMutexFn request,
                                   void (*destroy)(void *), const char *address, FVM_Mutex **out)
{
    FVM_PortSync *sync = fvm_port_sync_create();
    if (!sync)
        return FVM_OUT_OF_MEMORY;
    FVM_Result r = fvm_mutex_create(c, out);
    if (r == FVM_OK)
    {
        (*out)->remote_sync = sync;
        (*out)->origin = 0;
        (*out)->remote = remote;
        (*out)->request = request;
        (*out)->release_remote = destroy;
        (*out)->remote_address = address;
    }
    else
        fvm_port_sync_destroy(sync);
    return r;
}
const char *fvm_mutex_remote_address(FVM_Mutex *m)
{
    return m ? m->remote_address : 0;
}
extern "C" FVM_Result fvm_mutex_is_closed(FVM_Mutex *m, int *out)
{
    if (!m || !out)
        return FVM_INVALID_ARGUMENT;
    {
        RegistryGuard guard;
        *out = m->closed;
    }
    if (!*out && m->request)
        return m->request(m->remote, 0, 0, out);
    return FVM_OK;
}
extern "C" FVM_Result fvm_mutex_touch(FVM_Mutex *m, int *out)
{
    if (!m || !out)
        return FVM_INVALID_ARGUMENT;
    if (m->request)
    {
        {
            RegistryGuard guard;
            if (m->closed)
                return FVM_ERROR;
        }
        return m->request(m->remote, 0, 3, out);
    }
    return touch_impl(m, out);
}
extern "C" FVM_Result fvm_mutex_set(FVM_Context *c, FVM_Mutex *m, int blocking)
{
    if (!c || !m || c->destroying)
        return FVM_INVALID_ARGUMENT;
    FVM_Result r;
    if (!m->request)
        r = set_impl(c, m, blocking);
    else
    {
        do
        {
            int stop = 0, ignored = 0;
            if (c->worker)
                fvm_worker_stop_requested(c->worker, &stop);
            if (stop)
            {
                r = FVM_ERROR;
                break;
            }
            fvm_port_sync_lock(m->remote_sync);
            {
                RegistryGuard guard;
                r = m->closed || m->owner == c->identity || m->references == ULONG_MAX ? FVM_ERROR
                                                                                       : FVM_OK;
                if (r == FVM_OK)
                    ++m->references;
            }
            if (r == FVM_OK)
            {
                r = m->request(m->remote, c, 1, &ignored);
                RegistryGuard guard;
                if (r == FVM_OK)
                {
                    m->owner = c->identity;
                    m->owner_line = c->debug_line;
                    m->owner_offset = c->debug_offset;
                    std::memset(m->owner_source, 0, sizeof(m->owner_source));
                    std::memset(m->owner_function, 0, sizeof(m->owner_function));
                    std::strncpy(m->owner_source, c->debug_source.c_str(),
                                 sizeof(m->owner_source) - 1);
                    std::strncpy(m->owner_function, c->debug_function.c_str(),
                                 sizeof(m->owner_function) - 1);
                }
                else
                    drop(m);
            }
            fvm_port_sync_unlock(m->remote_sync);
            if (r == FVM_NOT_FOUND && blocking)
                fvm_port_thread_sleep_us(1000);
        } while (r == FVM_NOT_FOUND && blocking);
    }
    if (r == FVM_ERROR)
        fvm_debug_resource_error(c, m->identity, "mutex.set", m->remote_address);
    return r;
}
extern "C" FVM_Result fvm_mutex_reset(FVM_Context *c, FVM_Mutex *m)
{
    if (!c || !m || c->destroying)
        return FVM_INVALID_ARGUMENT;
    FVM_Result r;
    if (!m->request)
        r = reset_impl(c, m);
    else
    {
        fvm_port_sync_lock(m->remote_sync);
        int ignored = 0;
        {
            RegistryGuard guard;
            if (m->closed || m->owner != c->identity)
            {
                r = FVM_ERROR;
            }
            else
                r = FVM_OK;
        }
        if (r == FVM_OK)
            r = m->request(m->remote, c, 2, &ignored);
        if (r == FVM_OK)
        {
            RegistryGuard guard;
            m->owner = 0;
            drop(m);
        }
        fvm_port_sync_unlock(m->remote_sync);
    }
    if (r == FVM_ERROR)
        fvm_debug_resource_error(c, m->identity, "mutex.reset", m->remote_address);
    return r;
}
extern "C" FVM_Result fvm_mutex_close(FVM_Mutex *m)
{
    if (!m)
        return FVM_INVALID_ARGUMENT;
    if (m->remote_sync)
        fvm_port_sync_lock(m->remote_sync);
    FVM_Result result = FVM_OK;
    bool already;
    {
        RegistryGuard guard;
        already = m->closed;
    }
    if (!already && m->request)
    {
        int ignored = 0;
        result = m->request(m->remote, 0, 4, &ignored);
        if (result != FVM_OK)
            fvm_debug_resource_error(0, m->identity, "mutex.close", m->remote_address);
    }
    if (result == FVM_OK)
    {
        RegistryGuard guard;
        m->closed = true;
        if (m->owner)
        {
            m->owner = 0;
            drop(m);
        }
    }
    if (m->remote_sync)
        fvm_port_sync_unlock(m->remote_sync);
    return result;
}
extern "C" FVM_Result fvm_debug_set_location(FVM_Context *c, const char *source, unsigned long line)
{
    if (!c || !source || c->destroying)
        return FVM_INVALID_ARGUMENT;
    try
    {
        c->debug_source = source;
        c->debug_line = line;
        return FVM_OK;
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
extern "C" FVM_Result fvm_debug_mutex_snapshot(FVM_DebugMutex *out, size_t capacity, size_t *count)
{
    if (!count || (!out && capacity))
        return FVM_INVALID_ARGUMENT;
    RegistryGuard guard;
    size_t needed = 0;
    for (FVM_Mutex *m = mutexes; m; m = m->next)
    {
        if (!m->waiters)
            ++needed;
        for (MutexWait *w = m->waiters; w; w = w->next)
            ++needed;
    }
    *count = needed;
    if (capacity < needed)
        return FVM_NOT_FOUND;
    size_t index = 0;
    for (FVM_Mutex *m = mutexes; m; m = m->next)
    {
        if (!m->waiters)
            row(out[index++], m, 0, needed);
        for (MutexWait *w = m->waiters; w; w = w->next)
            row(out[index++], m, w, needed);
    }
    return FVM_OK;
}

namespace
{
struct ChannelWait
{
    FVM_DebugChannelWait info;
    ChannelWait *next;
};
ChannelWait *channel_waits = 0;
} // namespace
void *fvm_debug_channel_wait_begin(FVM_Context *c, FVM_Channel *ch, int sending)
{
    ChannelWait *w = new (std::nothrow) ChannelWait;
    if (!w)
        return 0;
    std::memset(&w->info, 0, sizeof(w->info));
    w->info.context = c->identity;
    w->info.channel = fvm_channel_identity(ch);
    w->info.sending = sending;
    w->info.line = c->debug_line;
    w->info.offset = c->debug_offset;
    std::strncpy(w->info.source, c->debug_source.c_str(), sizeof(w->info.source) - 1);
    std::strncpy(w->info.function, c->debug_function.c_str(), sizeof(w->info.function) - 1);
    RegistryGuard guard;
    w->next = channel_waits;
    channel_waits = w;
    return w;
}
void fvm_debug_channel_wait_end(void *raw)
{
    if (!raw)
        return;
    ChannelWait *w = (ChannelWait *)raw;
    RegistryGuard guard;
    ChannelWait **link = &channel_waits;
    while (*link != w)
        link = &(*link)->next;
    *link = w->next;
    delete w;
}
extern "C" FVM_Result fvm_debug_channel_wait_snapshot(FVM_DebugChannelWait *out, size_t capacity,
                                                      size_t *count)
{
    if (!count || (!out && capacity))
        return FVM_INVALID_ARGUMENT;
    RegistryGuard guard;
    size_t needed = 0;
    for (ChannelWait *w = channel_waits; w; w = w->next)
        ++needed;
    *count = needed;
    if (capacity < needed)
        return FVM_NOT_FOUND;
    size_t i = 0;
    for (ChannelWait *w = channel_waits; w; w = w->next)
        out[i++] = w->info;
    return FVM_OK;
}

extern "C" FVM_Result fvm_debug_map_action(FVM_Context *c, const char *function,
                                           unsigned long offset, const char *source,
                                           unsigned long line)
{
    if (!c || !function || !source || c->destroying)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_SourceLocation location;
        location.function = function;
        location.source = source;
        location.offset = offset;
        location.line = line;
        for (size_t i = 0; i < c->debug_locations.size(); ++i)
            if (c->debug_locations[i].function == location.function &&
                c->debug_locations[i].offset == offset)
            {
                c->debug_locations[i] = location;
                return FVM_OK;
            }
        c->debug_locations.push_back(location);
        return FVM_OK;
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
void fvm_debug_update_location(FVM_Context *c, unsigned long offset)
{
    c->debug_offset = offset;
    const FVM_SourceLocation *best = 0;
    for (size_t i = 0; i < c->debug_locations.size(); ++i)
    {
        const FVM_SourceLocation &location = c->debug_locations[i];
        if (location.function == c->debug_function && location.offset <= offset &&
            (!best || location.offset > best->offset))
            best = &location;
    }
    if (best)
    {
        c->debug_source = best->source;
        c->debug_line = best->line;
    }
}
