/* [GC:] FlashVM: exact accounting for context-owned counted string buffers without collection. */
#include "runtime.hpp"
#include "flashvm/gc.h"
#include "string_internal.hpp"
#include <cstring>
#include <new>
#include <climits>

/* [GC:] Sum physical payload/terminator bytes without reading text or changing arena ownership. */
extern "C" FVM_Result fvm_gc_string_stats(FVM_Context *c, FVM_GCStringStats *out)
{
    /* [GC:] Reject invalid or tearing-down contexts before observing their allocation vector. */
    if (!c || !out || c->destroying)
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Build an unpublished snapshot so overflow never modifies caller output. */
    FVM_GCStringStats result = {0, 0, 0};
    /* [GC:] Count only registered allocations, not unused descriptor vector capacity. */
    result.buffers = c->strings.size();
    /* [GC:] Traverse immutable descriptors on the owning VM thread without invoking callbacks. */
    for (size_t i = 0; i < c->strings.size(); ++i)
    {
        /* [GC:] Use stored byte counts rather than strlen, which loses embedded NUL payloads. */
        const FVM_ContextString &entry = c->strings[i];
        /* [GC:] Null active descriptors violate the ownership invariant. */
        if (!entry.data)
            return FVM_ERROR;
        /* [GC:] Check each addition before computing allocation length or accumulated totals. */
        if (entry.length == (size_t)-1 || entry.length > (size_t)-1 - result.payload_bytes ||
            entry.length + 1 > (size_t)-1 - result.allocated_bytes)
            return FVM_UNSUPPORTED;
        /* [GC:] Account exact copied payload bytes, including UTF-16 surrogate encodings. */
        result.payload_bytes += entry.length;
        /* [GC:] Each tracked allocation reserves its counted payload and one final NUL. */
        result.allocated_bytes += entry.length + 1;
    }
    /* [GC:] Publish the complete scalar snapshot only after all descriptors are validated. */
    *out = result;
    return FVM_OK;
}

/* [GC:] Pointer-sized integer keys must represent every supported flat-address target pointer. */
typedef char FVM_StringPointerWidth[sizeof(size_t) >= sizeof(void *) ? 1 : -1];

/* [GC:] Register both ownership indexes atomically; buffer cleanup stays with caller on failure. */
void fvm_context_adopt_string(FVM_Context *c, char *data, size_t length)
{
    /* [GC:] Index exact allocation starts without reading external or resource-owned memory. */
    size_t address = reinterpret_cast<size_t>(data);
    /* [GC:] Publish the future descriptor index before vector growth can fail. */
    c->string_allocations[address] = c->strings.size();
    /* [GC:] Roll back the ownership map if descriptor allocation fails. */
    try
    {
        /* [GC:] One descriptor records ownership and counted payload length together. */
        c->strings.push_back(FVM_ContextString(data, length));
        /* [GC:] String-only workloads contribute to automatic scheduling in traced mode. */
        if (c->gc_strings_traced && c->gc_allocations < ULONG_MAX)
            /* [GC:] Charge this successful traced-string allocation without counter overflow. */
            ++c->gc_allocations;
    }
    catch (...)
    {
        /* [GC:] No descriptor owns the buffer when insertion did not finish. */
        c->string_allocations.erase(address);
        throw;
    }
}

/* [GC:] Forget an explicitly rolled-back buffer before its address can be reused by an allocator.
 */
void fvm_context_forget_string(FVM_Context *c, const char *data)
{
    /* [GC:] Erasing integer metadata does not allocate or dereference the released payload. */
    c->string_allocations.erase(reinterpret_cast<size_t>(data));
}

/* [GC:] Snapshot successful native string views while their receiver/capture roots remain active.
 */
FVM_Result fvm_native_result(FVM_Context *c, FVM_Result status, FVM_Value *value)
{
    /* [GC:] Preserve errors and nonstring values without introducing unrelated allocations. */
    if (status != FVM_OK || !value || value->type != FVM_VALUE_STRING)
        return status;
    /* [GC:] Validate the returned string and any primitive metatable before retaining it. */
    FVM_Result result = fvm_validate_value(c, value);
    if (result != FVM_OK)
        return result;
    /* [GC:] Keep counted NUL/surrogate bytes intact; legacy terminated results resolve their count.
     */
    size_t length = fvm_string_size(*value);
    /* [GC:] Reuse known context storage rather than duplicating each getter/function result. */
    fvm_detail::map<size_t, size_t>::iterator owned =
        c->string_allocations.find(reinterpret_cast<size_t>(value->string));
    if (owned != c->string_allocations.end())
    {
        /* [GC:] Reject a native view extending beyond its registered owned buffer. */
        return owned->second < c->strings.size() &&
                       c->strings[owned->second].data == value->string &&
                       length <= c->strings[owned->second].length
                   ? FVM_OK
                   : FVM_INVALID_ARGUMENT;
    }
    /* [GC:] Check terminator growth before allocating a snapshot of foreign/resource-owned data. */
    if (length == (size_t)-1)
        return FVM_OUT_OF_MEMORY;
    /* [GC:] The local buffer remains unpublished until both ownership indexes accept it. */
    char *copy = new (std::nothrow) char[length + 1];
    if (!copy)
        return FVM_OUT_OF_MEMORY;
    /* [GC:] Copy while the native call barrier still retains the original resource owner. */
    std::memcpy(copy, value->string, length);
    /* [GC:] Preserve C terminated access without discarding counted embedded NUL bytes. */
    copy[length] = 0;
    /* [GC:] Ensure allocation failure cannot leave a dangling index or an unowned snapshot. */
    try
    {
        /* [GC:] Transfer the completed snapshot to the context. */
        fvm_context_adopt_string(c, copy, length);
    }
    catch (...)
    {
        /* [GC:] The caller still owns the unpublished buffer after failed adoption. */
        delete[] copy;
        return FVM_OUT_OF_MEMORY;
    }
    /* [GC:] Replace only string storage fields; preserve primitive metatable and other value state.
     */
    value->string = copy;
    /* [GC:] Retain the exact byte count independently of the native owner's lifetime. */
    value->string_size = length;
    return FVM_OK;
}
