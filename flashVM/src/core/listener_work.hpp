/* FlashVM: budgeted identity registration for context-owned input listener vectors. */
#ifndef FVM_LISTENER_WORK_HPP
#define FVM_LISTENER_WORK_HPP /* Guard the shared C++98 listener work helpers. */
#include "execution_work_scope.hpp"
#include <new>

/* Charge identity scans and worst-case vector relocation before retaining a new root. */
inline FVM_Result fvm_listener_add_budgeted(FVM_Context *context,
    fvm_detail::vector<FVM_Object *> &listeners, FVM_Object *listener, size_t limit)
{
    FVM_ExecutionWorkScope work(context);
    for (size_t i = 0; i < listeners.size(); ++i)
    {
        FVM_Result result = work.step();
        if (result != FVM_OK)
            return result;
        if (listeners[i] == listener)
            return FVM_OK;
    }
    if (listeners.size() >= limit)
        return FVM_ERROR;
    /* Reserve a work unit for each possible relocation, independent of spare capacity. */
    for (size_t i = 0; i < listeners.size(); ++i)
    {
        FVM_Result result = work.step();
        if (result != FVM_OK)
            return result;
    }
    FVM_Result result = work.step();
    if (result != FVM_OK)
        return result;
    try
    {
        /* [GC:] No callbacks or collection occur before the context-owned vector retains identity. */
        listeners.push_back(listener);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return FVM_ERROR;
    }
}

/* Precharge compaction; errors preserve both registration roots and the caller's output. */
inline FVM_Result fvm_listener_remove_budgeted(FVM_Context *context,
    fvm_detail::vector<FVM_Object *> &listeners, FVM_Object *listener, int *removed)
{
    FVM_ExecutionWorkScope work(context);
    for (size_t i = 0; i < listeners.size(); ++i)
    {
        FVM_Result result = work.step();
        if (result != FVM_OK)
            return result;
        if (listeners[i] != listener)
            continue;
        for (size_t j = i + 1; j < listeners.size(); ++j)
        {
            result = work.step();
            if (result != FVM_OK)
                return result;
        }
        /* [GC:] Pointer compaction cannot allocate, call script or collect after publication. */
        listeners.erase(listeners.begin() + i);
        *removed = 1;
        return FVM_OK;
    }
    *removed = 0;
    return FVM_OK;
}
#endif
