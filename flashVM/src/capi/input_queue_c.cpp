/* FlashVM: C embedding API: input queue.
 * File: src/capi/input_queue_c.cpp.
 */

#include "../core/runtime.hpp"
#include "../core/input_validation.hpp"
extern "C"
{
    /* Compute and return input enqueue. */
    FVM_Result fvm_input_enqueue(FVM_Context *c, const FVM_InputEvent *e, FVM_InputPhase phase)
    {
        FVM_QueuedInput item;
        if (!c || !fvm_input_event_valid(e) || !fvm_input_phase_valid(phase))
            return FVM_INVALID_ARGUMENT;
        if (c->queued_input.size() >= FVM_INPUT_MAX_PENDING_EVENTS)
            return FVM_ERROR;

        /* Computation / Вычисление и обновление состояния. */
        item.event = *e;
        item.phase = phase;
        try
        {
            c->queued_input.push_back(item);
        }
        catch (...)
        {
            return FVM_OUT_OF_MEMORY;
        }
        return FVM_OK;
    }
    /* Compute and return input pending. */
    FVM_Result fvm_input_pending(FVM_Context *c, unsigned int *out)
    {
        if (!c || !out)
            return FVM_INVALID_ARGUMENT;
        *out = (unsigned int)c->queued_input.size();
        return FVM_OK;
    }
    /* Compute and return input pump. */
    FVM_Result fvm_input_pump(FVM_Context *c, unsigned int limit, unsigned int *processed)
    {
        unsigned int count = 0;
        if (processed)
            *processed = 0;
        if (!c || c->pumping_input)
            return FVM_INVALID_ARGUMENT;
        c->pumping_input = true;
        /* Remove before invoking callbacks; callbacks may enqueue more work. */
        while (!c->queued_input.empty() && (!limit || count < limit))
        {
            FVM_QueuedInput item = c->queued_input.front();
            c->queued_input.erase(c->queued_input.begin());
            FVM_Result r = fvm_input_dispatch(c, &item.event, item.phase);
            if (r != FVM_OK)
            {
                c->pumping_input = false;
                if (processed)
                    *processed = count;
                return r;
            }
            ++count;
        }
        c->pumping_input = false;
        if (processed)
            *processed = count;
        return FVM_OK;
    }
}
