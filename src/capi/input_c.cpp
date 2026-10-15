/* FlashVM: C embedding API: input.
 * File: src/capi/input_c.cpp.
 */

#include "../core/runtime.hpp"
#include "../core/input_validation.hpp"
#include <new>
extern "C"
{
    /* Update input rule. */
    FVM_Result fvm_input_set_rule(FVM_Context *c, const FVM_InputRule *rule)
    {
        size_t i;
        if (!c || !rule || rule->struct_size != sizeof(FVM_InputRule) || rule->fingers == 0 ||
            rule->fingers > FVM_INPUT_MAX_FINGERS || rule->gesture < FVM_GESTURE_TAP ||
            rule->gesture > FVM_GESTURE_WHEEL || rule->button < FVM_MOUSE_NONE ||
            rule->button > FVM_MOUSE_WHEEL_DOWN)
            return FVM_INVALID_ARGUMENT;
        for (i = 0; i < c->input_rules.size(); ++i)
        {
            FVM_InputRule &r = c->input_rules[i];
            if (r.gesture == rule->gesture && r.fingers == rule->fingers &&
                r.direction == rule->direction)
            {
                r = *rule;
                return FVM_OK;
            }
        }
        try
        {
            c->input_rules.push_back(*rule);
        }
        catch (...)
        {
            return FVM_OUT_OF_MEMORY;
        }
        return FVM_OK;
    }
    /* Compute and return input map. */
    FVM_Result fvm_input_map(FVM_Context *c, const FVM_InputEvent *e, FVM_MouseButton *out)
    {
        size_t i;
        if (!c || !out || !fvm_input_event_valid(e))
            return FVM_INVALID_ARGUMENT;
        *out = FVM_MOUSE_NONE;
        /* Exact direction wins over wildcard; last registered rule wins for ties. */
        for (i = c->input_rules.size(); i > 0; --i)
        {
            const FVM_InputRule &r = c->input_rules[i - 1];
            if (r.gesture == e->gesture && r.fingers == e->fingers && r.direction == e->direction)
            {
                *out = r.button;
                return FVM_OK;
            }
        }
        for (i = c->input_rules.size(); i > 0; --i)
        {
            const FVM_InputRule &r = c->input_rules[i - 1];
            if (r.gesture == e->gesture && r.fingers == e->fingers && r.direction == 0)
            {
                *out = r.button;
                return FVM_OK;
            }
        }
        return FVM_OK;
    }
}

/* Update input callback. */
extern "C" FVM_Result fvm_input_set_callback(FVM_Context *c, FVM_InputCallback cb, void *userdata)
{
    if (!c)
        return FVM_INVALID_ARGUMENT;

    /* Computation / Вычисление и обновление состояния. */
    c->input_callback = cb;
    c->input_userdata = userdata;
    return FVM_OK;
}

/* Dispatch input. */
extern "C" FVM_Result fvm_input_dispatch(FVM_Context *c, const FVM_InputEvent *event,
                                         FVM_InputPhase phase)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_MouseButton button;
    FVM_Result result;
    if (!c || !fvm_input_phase_valid(phase))
        return FVM_INVALID_ARGUMENT;
    result = fvm_input_map(c, event, &button);
    if (result != FVM_OK)
        return result;
    result = fvm_input_set_pointer(c, event->x, event->y);
    if (result != FVM_OK)
        return result;
    try
    {
        if (c->input_callback)
            c->input_callback(c, event, phase, button, c->input_userdata);
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
