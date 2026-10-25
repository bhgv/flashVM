/* FlashVM: interpreter-only breakpoint dispatch and per-context JIT exclusion. */
#include "mode_internal.hpp"
#include "flashvm/source_debug.h"


/* A nonempty breakpoint list excludes the whole named body, including cached native bodies. */
bool fvm_debug_has_breakpoints(FVM_Context *c, const fvm_detail::string &name)
{
    return c->execution_mode.breakpoints.find(name) != c->execution_mode.breakpoints.end();
}


/* Add/remove on the owning VM thread; updates do not allocate executable memory or flush caches. */
extern "C" FVM_Result fvm_debug_set_breakpoint(FVM_Context *c, const char *function, unsigned long offset, int enabled)
{
    if (!c || c->destroying || c->gc_releasing || !function || (enabled != 0 && enabled != 1))
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {


        fvm_detail::string name(function);
        typedef fvm_detail::map<fvm_detail::string, fvm_detail::vector<unsigned long> > Points;
        Points &points = c->execution_mode.breakpoints;
        Points::iterator found = points.find(name);
        if (found != points.end())
        {
            fvm_detail::vector<unsigned long> &offsets = found->second;
            for (size_t i = 0; i < offsets.size(); ++i)
            {
                if (offsets[i] == offset)
                {
                    if (!enabled)
                    {
                        offsets[i] = offsets.back();
                        offsets.pop_back();
                        if (offsets.empty())
                        {
                            points.erase(found->first);
                        }
                    }
                    return FVM_OK;
                }
            }
            if (enabled)
            {
                offsets.push_back(offset);
            }
        }
        else if (enabled)
        { /* Construct off-map: allocation failure cannot leave an empty exclusion behind. */
            fvm_detail::vector<unsigned long> offsets;
            offsets.push_back(offset);
            points[name].swap(offsets);
        }
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}


/* The embedding application owns callback state until replacement or context destruction. */
extern "C" FVM_Result
fvm_debug_set_breakpoint_handler(FVM_Context *c, FVM_DebugBreakpointHandler callback, void *userdata)
{
    if (!c || c->destroying || c->gc_releasing)
    {
        return FVM_INVALID_ARGUMENT;
    }
    c->execution_mode.breakpoint_handler = callback;
    c->execution_mode.breakpoint_data = userdata;
    return FVM_OK;
}


/* Called only at interpreted action boundaries; no callback is executed from a native block. */
FVM_Result fvm_debug_breakpoint_poll(FVM_Context *c)
{
    FVM_ExecutionModeState &state = c->execution_mode;
    if (!state.breakpoint_handler || state.breakpoint_busy || state.replay_depth)
    {
        return FVM_OK;
    }
    fvm_detail::map<fvm_detail::string, fvm_detail::vector<unsigned long> >::iterator found =
        state.breakpoints.find(c->debug_function);
    if (found == state.breakpoints.end())
    {
        return FVM_OK;
    }
    bool hit = false;
    for (size_t i = 0; i < found->second.size(); ++i)
    {
        if (found->second[i] == c->debug_offset)
        {
            hit = true;
            break;
        }
    }
    if (!hit)
    {
        return FVM_OK;
    }
    FVM_DebugBreakpoint location;
    location.function = c->debug_function.c_str();
    location.source = c->debug_source.c_str();
    location.offset = c->debug_offset;
    location.line = c->debug_line;
    /* Source-content lookup occurs only on a breakpoint hit, not at each checkpoint. */
    FVM_DebugSourcePosition source;
    if (fvm_debug_current_source(c, &source) == FVM_OK)
    {
        location.source = source.source;
        location.line = source.line;
    }
    state.breakpoint_busy = true;
    FVM_Result result;
    try
    {
        result = state.breakpoint_handler(c, &location, state.breakpoint_data);
    }
    catch (const std::bad_alloc &)
    {
        result = FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        result = FVM_ERROR;
    }
    state.breakpoint_busy = false;
    return result;
}
