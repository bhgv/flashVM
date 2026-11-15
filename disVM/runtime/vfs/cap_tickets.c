/* Copyright (c) 2026 bhgv
 * EN: Serialized requester status, one-use grant transfer and cancellation.
 * RU: Serialized requester status, one-use grant transfer и cancellation.
 */
#include "cap_internal.h"


/* EN/RU: Refresh a pending native ticket exactly once under its state lock.
 * Обновить pending native ticket под state lock, grant удержать в native state. */
static void refresh(DVM_CapState *state)
{
    if (state->ticket && state->status == 0)
    {
        state->status = dvm_export_take(state->ticket, &state->root);
    }
}


/* EN/RU: Observe without consuming cached grant; aliases share native terminal state.
 * Наблюдать без consuming cached grant; aliases делят native terminal state. */
void Vfsexport_status(void *frame)
{
    F_Vfsexport_status call;
    WORD result = -1;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_status, ret), sizeof(*call.ret));
    DVM_CapState *state = dvm_cap_get(call.ticket, DVM_CAP_TICKET);
    if (state)
    {
        int released = dvm_runtime_gc_native_try_leave();
        if (released >= 0)
        {
            fvm_port_sync_lock(state->lock);
            refresh(state);
            result = state->status;
            fvm_port_sync_unlock(state->lock);
            dvm_runtime_gc_native_enter(released);
        }
    }
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}


/* EN/RU: Allocate rooted destination before consuming grant; native effect is never replayed.
 * Rooted destination создать до consuming grant; native effect не повторяется. */
void Vfsexport_take(void *frame)
{
    F_Vfsexport_take call;
    DVM_CapState *output;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_take, ret), sizeof(*call.ret));
    DVM_CapState *state = dvm_cap_get(call.ticket, DVM_CAP_TICKET);
    void *value = state ? dvm_cap_new(DVM_CAP_ROOT, &output) : H;
    if (waserror())
    {
        dvm_cap_abandon(value);
        nexterror();
    }
    if (value != H)
    {
        int released = dvm_runtime_gc_native_try_leave();
        if (released >= 0)
        {
            fvm_port_sync_lock(state->lock);
            refresh(state);
            if (state->status == 1)
            {
                output->root = state->root;
                state->root = NULL;
                state->status = -1;
            }
            fvm_port_sync_unlock(state->lock);
            dvm_runtime_gc_native_enter(released);
        }
    }
    int success = value != H && output->root;
    poperror();
    if (!success)
    {
        dvm_cap_abandon(value);
        value = H;
    }
    dvm_cap_publish(value, call.ret);
}


/* EN/RU: Detach ticket/grant under lock and retire outside it; future aliases see consumed.
 * Отделить ticket/grant под lock, освободить вне него; aliases видят consumed. */
void Vfsexport_cancel(void *frame)
{
    F_Vfsexport_cancel call;
    WORD result = 0;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsexport_cancel, ret), sizeof(*call.ret));
    DVM_CapState *state = dvm_cap_get(call.ticket, DVM_CAP_TICKET);
    if (state)
    {
        int released = dvm_runtime_gc_native_try_leave();
        if (released >= 0)
        {
            fvm_port_sync_lock(state->lock);
            DVM_ExportTicket *ticket = state->ticket;
            DVM_Provider *root = state->root;
            state->ticket = NULL;
            state->root = NULL;
            state->status = -1;
            result = ticket != NULL;
            fvm_port_sync_unlock(state->lock);
            dvm_export_release(ticket);
            dvm_provider_release(root);
            dvm_runtime_gc_native_enter(released);
        }
    }
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}
