#include "../core/runtime.hpp"
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
extern "C" FVM_Result fvm_mutex_touch(FVM_Mutex *m, int *out)
{
    if (!m || !out)
        return FVM_INVALID_ARGUMENT;
    RegistryGuard guard;
    *out = m->owner != 0;
    return FVM_OK;
}
extern "C" FVM_Result fvm_mutex_reset(FVM_Context *c, FVM_Mutex *m)
{
    if (!c || !m || c->destroying)
        return FVM_INVALID_ARGUMENT;
    RegistryGuard guard;
    if (m->owner != c->identity)
        return FVM_ERROR;
    m->owner = 0;
    drop(m);
    return FVM_OK;
}
extern "C" FVM_Result fvm_mutex_set(FVM_Context *c, FVM_Mutex *m, int blocking)
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
    if (m->owner == c->identity)
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
    while (m->owner)
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
    RegistryGuard guard;
    FVM_Mutex *m = mutexes;
    while (m)
    {
        FVM_Mutex *next = m->next;
        if (m->owner == c->identity)
        {
            m->owner = 0;
            drop(m);
        }
        m = next;
    }
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
