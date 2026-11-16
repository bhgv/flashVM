/* Copyright (c) 2026 bhgv
 * EN: Immutable conservative cancellation capability for retained path selections.
 * RU: Immutable conservative cancellation capability для retained path selections.
 */
#ifndef DVM_VFS_PATH_CANCEL_H
#define DVM_VFS_PATH_CANCEL_H
#include "resource.h"


/* EN/RU: Require every candidate to be an opt-in native provider; no I/O/permissions.
 * Mixed/native-host selections keep legacy admission. Plan/source pins are borrowed.
 * Требовать opt-in native provider во всех candidates; без I/O/прав;
 * mixed/native-host selections сохраняют legacy admission, plan borrowed. */
static inline int dvm_path_cancellable(const DVM_ResourcePlan *plan)
{
    if (!plan)
    {
        return 0;
    }
    for (; plan; plan = plan->layers)
    {
        if (!(dvm_provider_native_flags(plan->provider) & DVM_PROVIDER_CANCEL_PATH))
        {
            return 0;
        }
    }
    return 1;
}

/* EN/RU: Require PATH for every candidate and CAPTURE where acquiring a fresh node can call back.
 * Borrow plans; immutable checks only, no I/O. Stable aliases need no capture callback.
 * Требовать PATH в каждой ветви и CAPTURE при fresh callback; plans borrowed,
 * без I/O. Stable aliases не вызывают capture callback. Return1/0. */
static inline int dvm_capture_cancellable(const DVM_ResourcePlan *plan)
{
    if (!dvm_path_cancellable(plan))
    {
        return 0;
    }
    for (; plan; plan = plan->layers)
    {
        if (!dvm_provider_bindable_at(plan->provider, plan->relative) &&
            !(dvm_provider_native_flags(plan->provider) & DVM_PROVIDER_CANCEL_CAPTURE))
        {
            return 0;
        }
    }
    return 1;
}
#endif
