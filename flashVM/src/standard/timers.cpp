/* FlashVM: bounded, explicitly polled AS1/AS2 timers with strong pending and active roots. */
#include "timers.hpp"
#include "timer_constants.h"
#include "flashvm/timers.h"
#include "flashvm/execution.h"
#include "../core/runtime.hpp"
#include "../core/string_internal.hpp"
#include "../core/value_vector_root_scope.hpp"
#include "../core/execution_work_scope.hpp"
#include "../avm1/coercion.hpp"
#include <cmath>
#include <new>
#include <limits>

namespace
{
/* Reject lifecycle phases before linking roots, running coercion or touching clock state. */
bool usable(FVM_Context *c)
{
    return c && !c->destroying && !c->gc_collecting && !c->gc_releasing;
}
/* Initialize builtin outputs and enforce availability at invocation, including install-before-load. */
FVM_Result prepare(FVM_Context *c, unsigned version, unsigned count, const FVM_Value *args,
                   FVM_Value *out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    fvm_value_undefined(out);
    if (!usable(c) || (count && !args))
        return FVM_INVALID_ARGUMENT;
    return c->swf_loaded && c->swf_info.version < version ? FVM_UNSUPPORTED : FVM_OK;
}
/* [GC:] Snapshot borrowed strings and validate same-context objects before retaining a value. */
FVM_Result append(FVM_Context *c, FVM_ScheduledCall &record, const FVM_Value &value, FVM_ExecutionWorkScope &work)
{
    FVM_Result valid = fvm_validate_value(c, &value);
    if (valid != FVM_OK)
        return valid;
    size_t length = value.type == FVM_VALUE_STRING ? fvm_string_size(value) : 0;
    if (value.type == FVM_VALUE_STRING)
        for (size_t i = 0; i < length; ++i)
        {
            FVM_Result charged = work.step();
            if (charged != FVM_OK)
                return charged;
        }
    record.values.push_back(value);
    return fvm_native_result(c, FVM_OK, &record.values[record.values.size() - 1]);
}
/* Stage a complete record before publishing its never-reused context-local identity. */
FVM_Result schedule(FVM_Context *c, unsigned count, const FVM_Value *args, FVM_Value *out,
                    bool repeat)
{
    FVM_Result result = prepare(c, repeat ? FVM_TIMERS_INTERVAL_VERSION : FVM_TIMERS_TIMEOUT_VERSION,
                                count, args, out);
    if (result != FVM_OK)
        return result;
    if (count < FVM_TIMERS_ARGUMENT_OFFSET)
        return FVM_INVALID_ARGUMENT;
    if (args[0].type != FVM_VALUE_OBJECT || !args[0].object || args[0].object->owner != c)
        return FVM_INVALID_ARGUMENT;
    bool method = !args[0].object->invocable.callback;
    unsigned offset = FVM_TIMERS_ARGUMENT_OFFSET;
    if (method)
    {
        if (!repeat || count <= FVM_TIMERS_ARGUMENT_OFFSET || args[1].type != FVM_VALUE_STRING ||
            !fvm_string_size(args[1]))
            return FVM_UNSUPPORTED;
        ++offset;
    }
    if (count - offset > FVM_TIMERS_MAX_ARGUMENTS)
        return FVM_UNSUPPORTED;
    try
    {
        FVM_ExecutionWorkScope work(c);
        FVM_ScheduledCall record;
        /* [GC:] Retain copied values through user delay conversion and all partial preparation. */
        FVM_ValueVectorRootScope roots(c, record.values);
        result = append(c, record, args[0], work);
        FVM_Value name;
        fvm_value_undefined(&name);
        if (result == FVM_OK)
            result = append(c, record, method ? args[1] : name, work);
        for (unsigned i = offset; result == FVM_OK && i < count; ++i)
        {
            result = work.step();
            if (result == FVM_OK)
                result = append(c, record, args[i], work);
        }
        if (result != FVM_OK)
            return result;
        double delay = 0;
        result = fvm_avm1_number(c, &args[offset - 1], &delay);
        if (result != FVM_OK)
            return result;
        /* Unsupported timing inputs are explicit, rather than silently invented Player rules. */
        if (!(delay >= FVM_TIMERS_MIN_DELAY && delay <= FVM_TIMERS_MAX_DELAY))
            return FVM_UNSUPPORTED;
        record.delay = std::floor(delay);
        record.repeat = repeat;
        record.method = method;
        if (c->scheduled_calls.size() >= FVM_TIMERS_MAX_PENDING || c->last_timer_id == FVM_TIMERS_MAX_ID)
            return FVM_ERROR;
        FVM_Value clock;
        result = fvm_exec_get_time(c, &clock);
        if (result != FVM_OK)
            return result;
        record.deadline = c->timer_elapsed_ms + record.delay;
        if (!(record.deadline > c->timer_elapsed_ms))
            return FVM_UNSUPPORTED;
        result = work.step();
        if (result != FVM_OK)
            return result;
        FVM_ScheduledCall *published = new FVM_ScheduledCall(record);
        unsigned long id = c->last_timer_id + 1;
        try
        {
            c->scheduled_calls[id] = published;
        }
        catch (...)
        {
            delete published;
            throw;
        }
        c->last_timer_id = id;
        fvm_value_number(out, (double)id);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
/* Cancel only after numeric coercion and work preflight; missing identifiers are harmless. */
FVM_Result cancel(FVM_Context *c, unsigned count, const FVM_Value *args, FVM_Value *out,
                  unsigned version)
{
    FVM_Result result = prepare(c, version, count, args, out);
    if (result != FVM_OK || !count)
        return result;
    try
    {
        FVM_ExecutionWorkScope work(c);
        double id = 0;
        result = fvm_avm1_number(c, &args[0], &id);
        if (result != FVM_OK)
            return result;
        result = work.step();
        if (result != FVM_OK)
            return result;
        if (!(id >= 1 && id <= (double)FVM_TIMERS_MAX_ID) || std::floor(id) != id)
            return FVM_OK;
        fvm_detail::map<unsigned long, FVM_ScheduledCall *>::iterator found =
            c->scheduled_calls.find((unsigned long)id);
        if (found == c->scheduled_calls.end())
            return FVM_OK;
        for (size_t i = 0; i < found->second->values.size() + c->scheduled_calls.size(); ++i)
        {
            result = work.step();
            if (result != FVM_OK)
                return result;
        }
        delete found->second;
        c->scheduled_calls.erase((unsigned long)id);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
/* Prevent nested polls and context teardown throughout snapshot construction and delivery. */
class DispatchScope
{
    FVM_Context *context_; /* Owning context whose active delivery flag is borrowed. */
    DispatchScope(const DispatchScope &); /* Keep flag ownership unique. */
    DispatchScope &operator=(const DispatchScope &); /* Keep restoration nesting unambiguous. */
  public:
    /* Enter after checking the context and rejecting a previous active poll. */
    explicit DispatchScope(FVM_Context *c) : context_(c) { c->timers_dispatching = true; }
    /* Restore on normal return, callback status and allocation unwinding. */
    ~DispatchScope() { context_->timers_dispatching = false; }
};
}
/* Register periodic work with Function and object/method overloads. */
FVM_Result fvm_builtin_set_interval(FVM_Context *c, FVM_Object *, unsigned n, const FVM_Value *a,
                                    FVM_Value *out, void *)
{
    return schedule(c, n, a, out, true);
}
/* Apply the SWF6 cancellation gate; IDs share the same context-local namespace. */
FVM_Result fvm_builtin_clear_interval(FVM_Context *c, FVM_Object *, unsigned n, const FVM_Value *a,
                                      FVM_Value *out, void *)
{
    return cancel(c, n, a, out, FVM_TIMERS_INTERVAL_VERSION);
}
/* Register one-shot Function work under SWF8 availability. */
FVM_Result fvm_builtin_set_timeout(FVM_Context *c, FVM_Object *, unsigned n, const FVM_Value *a,
                                   FVM_Value *out, void *)
{
    return schedule(c, n, a, out, false);
}
/* Apply the SWF8 cancellation gate to a scheduled identifier. */
FVM_Result fvm_builtin_clear_timeout(FVM_Context *c, FVM_Object *, unsigned n, const FVM_Value *a,
                                     FVM_Value *out, void *)
{
    return cancel(c, n, a, out, FVM_TIMERS_TIMEOUT_VERSION);
}
/* [GC:] Release retained pending values before heap objects are finalized; never invoke callbacks. */
void fvm_timers_cleanup(FVM_Context *c)
{
    for (fvm_detail::map<unsigned long, FVM_ScheduledCall *>::iterator it = c->scheduled_calls.begin();
         it != c->scheduled_calls.end(); ++it)
        delete it->second;
    c->scheduled_calls.clear();
}
/* Return a capped, upward-rounded wakeup hint without mutating or invoking pending calls. */
extern "C" FVM_Result fvm_timers_next_delay(FVM_Context *c, unsigned long max_wait_ms,
                                           unsigned long *wait_ms, int *pending)
{
    if (!usable(c) || !wait_ms || !pending)
        return FVM_INVALID_ARGUMENT;
    FVM_ExecutionWorkScope work(c);
    unsigned long wait = max_wait_ms;
    if (!c->scheduled_calls.empty())
    {
        FVM_Value clock;
        FVM_Result result = fvm_exec_get_time(c, &clock);
        if (result != FVM_OK)
            return result;
        /* Start with a real deadline: large elapsed values can round an accepted delay upward. */
        double nearest = c->scheduled_calls.begin()->second->deadline - c->timer_elapsed_ms;
        for (fvm_detail::map<unsigned long, FVM_ScheduledCall *>::iterator it =
                 c->scheduled_calls.begin(); it != c->scheduled_calls.end(); ++it)
        {
            result = work.step();
            if (result != FVM_OK)
                return result;
            double remaining = it->second->deadline - c->timer_elapsed_ms;
            if (remaining < nearest)
                nearest = remaining;
        }
        /* A due record returns zero; submillisecond remainders must not cause an early busy loop. */
        double rounded = nearest <= 0 ? 0 : std::ceil(nearest);
        /* Keep conversion inside the host integer range, including unusually large clock steps. */
        if (rounded < (double)std::numeric_limits<unsigned long>::max())
        {
            unsigned long candidate = (unsigned long)rounded;
            if (candidate < wait)
                wait = candidate;
        }
    }
    /* Publish both outputs only after the entire clock/scan/work operation succeeds. */
    *wait_ms = wait;
    *pending = c->scheduled_calls.empty() ? 0 : 1;
    return FVM_OK;
}
/* Snapshot eligibility once, then consume/rearm each still-live call before invoking user code. */
extern "C" FVM_Result fvm_timers_poll(FVM_Context *c, unsigned int *delivered)
{
    if (!usable(c) || !delivered || c->timers_dispatching)
        return FVM_INVALID_ARGUMENT;
    *delivered = 0;
    try
    {
        DispatchScope dispatch(c);
        FVM_ExecutionWorkScope work(c);
        if (c->scheduled_calls.empty())
            return FVM_OK;
        FVM_Value clock;
        FVM_Result result = fvm_exec_get_time(c, &clock);
        if (result != FVM_OK)
            return result;
        const double now = c->timer_elapsed_ms;
        fvm_detail::vector<unsigned long> due;
        for (fvm_detail::map<unsigned long, FVM_ScheduledCall *>::iterator it = c->scheduled_calls.begin();
             it != c->scheduled_calls.end(); ++it)
        {
            result = work.step();
            if (result != FVM_OK)
                return result;
            if (it->second->deadline <= now)
                due.push_back(it->first);
        }
        for (size_t i = 0; i < due.size(); ++i)
        {
            fvm_detail::map<unsigned long, FVM_ScheduledCall *>::iterator found =
                c->scheduled_calls.find(due[i]);
            if (found == c->scheduled_calls.end())
                continue;
            result = work.step();
            if (result != FVM_OK)
                return result;
            /* Precharge the bounded active snapshot before any consumption or rearming. */
            for (size_t j = 0; j < found->second->values.size() + (found->second->repeat ? 0 : c->scheduled_calls.size()); ++j)
            {
                result = work.step();
                if (result != FVM_OK)
                    return result;
            }
            FVM_ScheduledCall active(*found->second);
            /* [GC:] Active arguments and target survive self-cancellation and collecting callbacks. */
            FVM_ValueVectorRootScope roots(c, active.values);
            if (active.repeat)
            {
                double next = now + active.delay;
                if (!(next > now))
                    return FVM_UNSUPPORTED;
                found->second->deadline = next;
            }
            else
            {
                delete found->second;
                c->scheduled_calls.erase(due[i]);
            }
            ++*delivered;
            FVM_Value ignored;
            unsigned argc = (unsigned)(active.values.size() - FVM_TIMERS_ARGUMENT_OFFSET);
            const FVM_Value *argv = argc ? &active.values[FVM_TIMERS_ARGUMENT_OFFSET] : 0;
            if (active.method)
                result = fvm_exec_call_method(c, 0, &active.values[0], &active.values[1], 0, argc, argv, &ignored);
            else
                result = fvm_function_invoke(c, active.values[0].object, 0, argc, argv, &ignored);
            if (result != FVM_OK)
                return result;
        }
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
