/* [GC:] FlashVM: traced context strings with counted allocation ranges and transactional metadata.
 */
#include "string_gc.hpp"
#include "flashvm/gc.h"
/* [GC:] Reachable native fields also contribute roots after resource callbacks. */
#include "../standard/text_state.hpp"
#include <cstdlib>
#include <cstring>
#include <climits>

/* [GC:] Compare flat allocation addresses without subtraction overflow or pointer ordering UB. */
static int compare_string_ranges(const void *left, const void *right)
{
    /* [GC:] Read only copied allocation metadata; no callbacks or string bytes are accessed. */
    size_t a = ((const FVM_StringGCRange *)left)->address;
    size_t b = ((const FVM_StringGCRange *)right)->address;
    return a < b ? -1 : a > b ? 1 : 0;
}

/* [GC:] Preserve the legacy arena policy unless the embedding application explicitly opts in. */
extern "C" FVM_Result fvm_gc_string_mode(FVM_Context *c, int mode)
{
    /* [GC:] Policy transitions are allowed only on the owning VM thread outside
     * callbacks/collection. */
    if (!c || c->destroying || c->gc_collecting || c->gc_native_depth ||
        /* [GC:] Only documented arena/traced policies may be installed. */
        (mode != FVM_GC_STRINGS_ARENA && mode != FVM_GC_STRINGS_TRACED))
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Enabling tracing does not immediately collect existing borrowed buffers. */
    c->gc_strings_traced = mode == FVM_GC_STRINGS_TRACED;
    return FVM_OK;
}

/* [GC:] Query string ownership policy without allocating or running the collector. */
extern "C" FVM_Result fvm_gc_string_state(FVM_Context *c, int *out)
{
    /* [GC:] Invalid contexts/outputs must not change the caller's policy snapshot. */
    if (!c || !out || c->destroying)
        return FVM_INVALID_ARGUMENT;
    /* [GC:] Publish the current ownership contract only after validating the context. */
    *out = c->gc_strings_traced ? FVM_GC_STRINGS_TRACED : FVM_GC_STRINGS_ARENA;
    return FVM_OK;
}

/* [GC:] Allocate all range/mark storage before any finalizer or resource release can run. */
FVM_StringGCPlan::FVM_StringGCPlan(FVM_Context *c) : context_(c), enabled_(c->gc_strings_traced)
{
    /* [GC:] Legacy arena mode introduces no new collection allocations or string lifetime changes.
     */
    if (!enabled_)
        return;
    /* [GC:] Existing descriptors retain stable indexes even if finalizers append new allocations.
     */
    size_t count = c->strings.size();
    ranges_.resize(count);
    live_.resize(count);
    /* [GC:] Copy POD metadata rather than retaining pointers into the movable descriptor vector. */
    for (size_t i = 0; i < count; ++i)
    {
        ranges_[i].address = reinterpret_cast<size_t>(c->strings[i].data);
        ranges_[i].length = c->strings[i].length;
        ranges_[i].index = i;
    }
    /* [GC:] Sorting enables logarithmic lookup of exact and interior borrowed views. */
    if (count)
        std::qsort(ranges_.begin(), count, sizeof(FVM_StringGCRange), compare_string_ranges);
}

/* [GC:] Finalizer mutations require a fresh root traversal rather than retaining obsolete marks. */
void FVM_StringGCPlan::reset()
{
    /* [GC:] Avoid passing a null vector pointer to memset for an empty arena. */
    if (!live_.empty())
        std::memset(live_.begin(), 0, live_.size());
}

/* [GC:] Mark a registered buffer without dereferencing borrowed or external string pointers. */
void FVM_StringGCPlan::mark(const char *text)
{
    /* [GC:] External strings are not owned by this collector; empty legacy plans do no work. */
    if (!enabled_ || !text || ranges_.empty())
        return;
    /* [GC:] Find the last allocation whose start does not exceed the reachable view address. */
    size_t address = reinterpret_cast<size_t>(text), low = 0, high = ranges_.size();
    while (low < high)
    {
        size_t middle = low + (high - low) / 2;
        if (ranges_[middle].address <= address)
            low = middle + 1;
        else
            high = middle;
    }
    /* [GC:] A pointer preceding every registered allocation cannot own one of these buffers. */
    if (!low)
        return;
    const FVM_StringGCRange &range = ranges_[low - 1];
    /* [GC:] Include a view of the final terminator; subtraction avoids address-plus-length
     * overflow. */
    if (address - range.address <= range.length)
        live_[range.index] = 1;
}

/* [GC:] Native resource callbacks may copy old views into existing root stacks after remark. */
void FVM_StringGCPlan::retain_release_roots()
{
    /* [GC:] Arena mode requires no additional root work. */
    if (!enabled_)
        return;
    /* [GC:] A stack API push stores a borrowed value, so trace updated registered stacks again. */
    for (size_t i = 0; i < context_->gc_stacks.size(); ++i)
        /* [GC:] Revisit every current entry, including values appended by release callbacks. */
        for (size_t j = 0; j < context_->gc_stacks[i]->size(); ++j)
        {
            /* [GC:] Borrow the registered root value without copying or allocating text. */
            const FVM_Value &value = (*context_->gc_stacks[i])[j];
            if (value.type == FVM_VALUE_STRING)
                mark(value.string);
        }
    /* [GC:] Preserve any string roots published into still-active interpreter/API frames. */
    for (FVM_GCRootFrame *frame = context_->gc_frames; frame; frame = frame->previous)
    {
        if (frame->stack)
            for (size_t i = 0; i < frame->stack->size(); ++i)
                if ((*frame->stack)[i].type == FVM_VALUE_STRING)
                    mark((*frame->stack)[i].string);
        if (frame->locals)
            for (fvm_detail::map<fvm_detail::string, FVM_Value>::iterator value =
                     frame->locals->begin();
                 value != frame->locals->end(); ++value)
                if (value->second.type == FVM_VALUE_STRING)
                    mark(value->second.string);
    }
    /* [GC:] Newly published exception string roots must survive the release phase as well. */
    if (context_->exception_pending && context_->exception_value.type == FVM_VALUE_STRING)
        mark(context_->exception_value.string);
    /* [GC:] Native field/property updates may retain old views in already-surviving objects. */
    for (size_t i = 0; i < context_->objects.size(); ++i)
    {
        FVM_Object *object = context_->objects[i];
        /* [GC:] Only surviving managed objects retain native/property string storage here. */
        if (!object->gc_marked)
            continue;
        mark(fvm_textfield_string_root(object));
        if (object->resource_result_valid && object->resource_result.type == FVM_VALUE_STRING)
            mark(object->resource_result.string);
        for (fvm_detail::map<fvm_detail::string, FVM_Value>::iterator value =
                 object->properties.begin();
             value != object->properties.end(); ++value)
            if (value->second.type == FVM_VALUE_STRING)
                mark(value->second.string);
        for (fvm_detail::map<unsigned long, FVM_Value>::iterator value = object->indexed.begin();
             value != object->indexed.end(); ++value)
            if (value->second.type == FVM_VALUE_STRING)
                mark(value->second.string);
    }
}

/* [GC:] Reclaim only original unmarked strings; finalizer-created buffers survive this collection.
 */
void FVM_StringGCPlan::sweep()
{
    /* [GC:] Arena mode continues retaining every buffer until context destruction. */
    if (!enabled_)
        return;
    size_t kept = 0;
    /* [GC:] No allocation or callback is allowed once irreversible buffer release starts. */
    for (size_t i = 0; i < context_->strings.size(); ++i)
    {
        FVM_ContextString entry = context_->strings[i];
        size_t address = reinterpret_cast<size_t>(entry.data);
        if (i < live_.size() && !live_[i])
        {
            /* [GC:] Remove the address index before freeing memory that an allocator may reuse. */
            context_->string_allocations.erase(address);
            delete[] entry.data;
        }
        else
        {
            /* [GC:] Update an existing map entry without insertion, allocation or rehashing. */
            context_->string_allocations.find(address)->second = kept;
            context_->strings[kept++] = entry;
        }
    }
    /* [GC:] Shrinking POD descriptors does not allocate and drops stale ownership metadata. */
    context_->strings.resize(kept);
}
