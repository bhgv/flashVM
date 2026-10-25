/* Preserve messages across independent channel backpressure and service-owner scheduling. */
#include "disvm/ui_pump.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>


/* Only the owning service thread mutates this routing state. */
struct DVM_UIPump
{
    DVM_UIBridge *bridge;  /* Borrowed transport, alive through destruction. */
    DVM_UIOffer offer;     /* Nonblocking service ingress callback. */
    DVM_UITake take;       /* Nonblocking service egress callback. */
    void *opaque;          /* Borrowed endpoint state, never sent over channels. */
    DVM_UIMessage ingress; /* Copied request/input awaiting service acceptance. */
    DVM_UIMessage egress;  /* Copied service output awaiting UI queue capacity. */
    DVM_UIPumpState state; /* Counters, retained flags and first terminal failure. */
    int active;            /* Reject same-owner callback reentry. */
};


/* Increment an acceptance counter without reusing a wrapped value.
 * count is a writable counter. Returns nothing; ULONG_MAX remains saturated. */
static void accepted(unsigned long *count)
{
    if (*count != ULONG_MAX)
    {
        (*count)++;
    }
}


/* Allocate routing state after validating all required callbacks and output.
 * Parameters and return ownership follow ui_pump.h; no channel reference is transferred. */
FVM_Result dvm_ui_pump_create(DVM_UIBridge *bridge, DVM_UIOffer offer, DVM_UITake take, void *opaque, DVM_UIPump **out)
{
    DVM_UIPump *pump;
    if (!bridge || !offer || !take || !out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    pump = (DVM_UIPump *)calloc(1, sizeof(*pump));
    if (!pump)
    {
        return FVM_OUT_OF_MEMORY;
    }
    pump->bridge = bridge;
    pump->offer = offer;
    pump->take = take;
    pump->opaque = opaque;
    pump->state.failure = FVM_OK;
    *out = pump;
    return FVM_OK;
}


/* Try ingress once, retaining any already consumed request until explicit acceptance.
 * pump is active and owner-held; progress is set to one after any queue transfer.
 * Returns OK, NOT_FOUND for backpressure, or a terminal callback/transport status. */
static FVM_Result ingress(DVM_UIPump *pump, int *progress)
{
    FVM_Result result;
    if (!pump->state.pending_service)
    {
        result = dvm_ui_receive(pump->bridge, DVM_UI_DIS, &pump->ingress, 0);
        if (result != FVM_OK)
        {
            return result;
        }
        pump->state.pending_service = 1;
        *progress = 1;
    }
    result = pump->offer(pump->opaque, &pump->ingress);
    if (result == FVM_OK)
    {
        pump->state.pending_service = 0;
        accepted(&pump->state.to_service);
        *progress = 1;
    }
    return result;
}


/* Try egress once, retaining the exact service record when the UI queue is full.
 * pump/progress follow ingress. Returns OK, NOT_FOUND, or a terminal status. */
static FVM_Result egress(DVM_UIPump *pump, int *progress)
{
    FVM_Result result;
    if (!pump->state.pending_ui)
    {
        result = pump->take(pump->opaque, &pump->egress);
        if (result != FVM_OK)
        {
            return result;
        }
        pump->state.pending_ui = 1;
        *progress = 1;
    }
    result = dvm_ui_send(pump->bridge, DVM_UI_DIS, &pump->egress, 0);
    if (result == FVM_OK)
    {
        pump->state.pending_ui = 0;
        accepted(&pump->state.to_ui);
        *progress = 1;
    }
    return result;
}


/* Advance both owner boundaries without waiting or bypassing pending FIFO records.
 * Parameters, progress and terminal-state guarantees follow ui_pump.h. */
FVM_Result dvm_ui_pump_step(DVM_UIPump *pump, unsigned int budget, DVM_UIPumpState *state)
{
    unsigned int round;
    int any = 0;
    FVM_Result result = FVM_NOT_FOUND;
    if (!pump || !state || !budget || budget > DVM_UI_PUMP_BUDGET_MAX)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (pump->active)
    {
        return FVM_ERROR;
    }
    if (pump->state.stopped)
    {
        *state = pump->state;
        return pump->state.failure == FVM_OK ? FVM_ERROR : pump->state.failure;
    }
    pump->active = 1;
    for (round = 0; round < budget; round++)
    {
        int progress = 0;
        result = ingress(pump, &progress);
        if (result != FVM_OK && result != FVM_NOT_FOUND)
        {
            break;
        }
        result = egress(pump, &progress);
        if (result != FVM_OK && result != FVM_NOT_FOUND)
        {
            break;
        }
        if (!progress)
        {
            break;
        }
        any = 1;
    }
    if (result != FVM_OK && result != FVM_NOT_FOUND)
    {
        pump->state.stopped = 1;
        pump->state.failure = result;
    }
    else
    {
        result = any ? FVM_OK : FVM_NOT_FOUND;
    }
    pump->active = 0;
    *state = pump->state;
    return result;
}


/* Read inactive owner state; parameters/output behavior follow ui_pump.h. */
FVM_Result dvm_ui_pump_state(const DVM_UIPump *pump, DVM_UIPumpState *state)
{
    if (!pump || !state)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (pump->active)
    {
        return FVM_ERROR;
    }
    *state = pump->state;
    return FVM_OK;
}


/* Copy rather than consume a pending value, permitting diagnostics and explicit recovery.
 * Parameters and absence/error statuses follow ui_pump.h. */
FVM_Result dvm_ui_pump_pending(const DVM_UIPump *pump, int toward_service, DVM_UIMessage *out)
{
    if (!pump || !out || (toward_service != 0 && toward_service != 1))
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (pump->active)
    {
        return FVM_ERROR;
    }
    if (toward_service ? !pump->state.pending_service : !pump->state.pending_ui)
    {
        return FVM_NOT_FOUND;
    }
    *out = toward_service ? pump->ingress : pump->egress;
    return FVM_OK;
}


/* Stop future callbacks, wake queue waiters, and preserve retained records for inspection.
 * pump is owner-held; return statuses follow ui_pump.h. */
FVM_Result dvm_ui_pump_stop(DVM_UIPump *pump)
{
    if (!pump)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (pump->active)
    {
        return FVM_ERROR;
    }
    pump->state.stopped = 1;
    dvm_ui_close(pump->bridge);
    return FVM_OK;
}


/* Release owner state without taking ownership of its borrowed bridge or callbacks.
 * pump must be inactive; returns OK or the argument/reentry error described in ui_pump.h. */
FVM_Result dvm_ui_pump_destroy(DVM_UIPump *pump)
{
    if (!pump)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (pump->active)
    {
        return FVM_ERROR;
    }
    free(pump);
    return FVM_OK;
}
