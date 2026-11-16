/* Copyright (c) 2026 bhgv
 * EN: Private immutable native factory callbacks and independent capture-origin lifetime.
 * RU: Private immutable native callbacks factory и independent capture-origin lifetime.
 */
#ifndef DVM_PROVIDER_INTERNAL_H
#define DVM_PROVIDER_INTERNAL_H
#include "provider.h"
#include "channel_sync.h"


/* EN/RU: Refcount lock protects lifetime only; capture callbacks execute outside all locks.
 * Refcount lock защищает только lifetime; capture callbacks выполняются вне locks. */
struct DVM_Provider
{
    FVM_PortSync *lock; /* Native lifetime lock. */
    size_t references; /* Owning native capabilities/handles. */
    const DVM_ProviderOps *ops; /* Immutable callback table/code through destruction. */
    void *state; /* Owned native factory state. */
    DVM_Changes *changes; /* Owned local invalidation journal. */
    DVM_ProviderCapture capture; /* Optional immutable native node-capture callback. */
    unsigned native_flags; /* Immutable native control-operation cancellation promises. */
    int anchored; /* Dot names a fixed captured root; descendants still require capture. */
    DVM_Provider *origin; /* Owned source keeps its feed/policy alive, never a child backlink. */
    DVM_NoticeTap *origin_tap; /* Owned invalidation bridge, detached before source release. */
};


/* EN/RU: Detach capture-origin forwarding before factory retirement; no callback wait.
 * Отсоединить forwarding capture-origin до factory retirement; без callback wait. */
void dvm_provider_capture_detach(DVM_Provider *provider);
#endif
