#include "../core/capability_internal.hpp"
#include "../core/exception_internal.hpp"
#include "flashvm/debug.h"
#include "../../ports/channel_sync.h"
#include <cstring>
namespace
{
FVM_DebugResourceError errors[64];
size_t total = 0;
void copy(char *out, const char *s, size_t n)
{
    std::memset(out, 0, n);
    if (s)
        std::strncpy(out, s, n - 1);
}
} // namespace
void fvm_debug_resource_error(FVM_Context *c, unsigned long id, const char *operation,
                              const char *address)
{
    fvm_resource_fault(c, FVM_RESOURCE_FAILURE, id, operation, address);
    FVM_DebugResourceError event;
    std::memset(&event, 0, sizeof(event));
    event.resource = id;
    event.result = FVM_ERROR;
    copy(event.operation, operation, sizeof(event.operation));
    copy(event.address, address, sizeof(event.address));
    if (c)
    {
        event.context = c->identity;
        event.line = c->debug_line;
        event.offset = c->debug_offset;
        copy(event.source, c->debug_source.c_str(), sizeof(event.source));
        copy(event.function, c->debug_function.c_str(), sizeof(event.function));
    }
    fvm_port_registry_lock();
    if (total == 64)
    {
        std::memmove(errors, errors + 1, sizeof(errors[0]) * 63);
        --total;
    }
    errors[total++] = event;
    fvm_port_registry_unlock();
}
extern "C" FVM_Result fvm_debug_resource_errors(FVM_DebugResourceError *out, size_t capacity,
                                                size_t *count, int clear)
{
    if (!count || (!out && capacity))
        return FVM_INVALID_ARGUMENT;
    fvm_port_registry_lock();
    *count = total;
    if (capacity < total)
    {
        fvm_port_registry_unlock();
        return FVM_NOT_FOUND;
    }
    if (total)
        std::memcpy(out, errors, total * sizeof(errors[0]));
    if (clear)
        total = 0;
    fvm_port_registry_unlock();
    return FVM_OK;
}
