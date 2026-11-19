/* Copyright (c) 2026 bhgv
 * EN: Private native launch state; independent wrappers retain shared immutable request data.
 * RU: Приватное native состояние запуска; wrappers удерживают immutable request data.
 */
#ifndef DVM_LAUNCH_TICKET_INTERNAL_H
#define DVM_LAUNCH_TICKET_INTERNAL_H
#include "ticket.h"
#include "../cancel.h"
#include "channel_sync.h"


/* EN/RU: Mutable state belongs to lock; immutable request/source pins live through both wrappers.
 * Mutable state под lock; request/source pins живут до завершения двух wrappers. */
typedef struct LaunchShared
{
    FVM_PortSync *lock; /* Protects state, group and references; never held across provider I/O. */
    unsigned references; /* Requester plus provider wrappers. */
    DVM_LaunchState state; /* Preparation/ownership state only. */
    DVM_LaunchDescription description; /* Immutable copied request fields. */
    DVM_LaunchContract *contract; /* Owned immutable requirements, NULL preserves legacy untyped admission. */
    int contract_checked; /* Lock-protected one-way match acceptance while PENDING, initially true only for untyped. */
    unsigned char *arguments; /* Immutable owned request bytes. */
    DVM_Provider *terminal; /* Owned granted root, never retained by cancellation callbacks. */
    DVM_TerminalChannels flashvm; /* Owned separate FlashVM channel grant until take/final retirement. */
    DVM_Cancel *cancel; /* Independent native preparation signal. */
    DVM_Changes *changes; /* Owned journal, external observers may retain it independently. */
    DVM_Namespace *group; /* Owned only in READY, detached under lock. */
    void (*retired)(void *); /* Optional admission-slot retirement before final storage release. */
    void (*changed)(void *); /* Optional native journal forwarding, outside request mutex. */
    void *context; /* Native admission slot only, never a guest pointer. */
    DVM_Origin provider_origin; /* Written once before READY publication. */
} LaunchShared;


/* EN/RU: Separate native wrapper types prevent accidental transfer of provider take authority.
 * Разные native wrapper types разделяют requester status/cancel и provider take. */
struct DVM_LaunchTicket
{
    LaunchShared *shared; /* Owned requester reference. */
};


/* EN/RU: Provider owns this wrapper until all preparation/take callers join.
 * Provider владеет wrapper до join всех preparation/take callers. */
struct DVM_LaunchRequest
{
    LaunchShared *shared; /* Owned provider reference. */
};


/* EN/RU: Query immutable root cancellation promises; no provider callbacks or ownership change.
 * Проверить immutable promises root без callbacks/смены ownership; return1/0. */
static inline int dvm_launch_root_cancellable(DVM_Provider *root)
{
    unsigned flags = dvm_provider_native_flags(root);
    return (flags & DVM_PROVIDER_CANCEL_PATH) && (dvm_provider_bindable_at(root, ".") ||
        (dvm_provider_capturable(root) && (flags & DVM_PROVIDER_CANCEL_CAPTURE)));
}


/* EN/RU: Retain a live shared record under an external index lifetime guard;1/0 on finalizing/overflow.
 * Удержать live record под external index lifetime guard;1/0 при finalizing/overflow. */
int dvm_launch_shared_retain(LaunchShared *shared);


/* EN/RU: Terminate unfinished shared request; signals/cleanup outside mutex,1 changed/0 terminal.
 * Завершить unfinished request, signals/cleanup вне mutex;1/0. */
int dvm_launch_shared_terminate(LaunchShared *shared, DVM_LaunchState state);


/* EN/RU: Consume shared ref after wrapper stops using it, freeing last owners outside lock.
 * Поглотить shared ref после завершения wrapper; last owners освобождаются вне lock. */
void dvm_launch_shared_drop(LaunchShared *shared);


/* EN/RU: Publish a native invalidation hint outside shared mutex; no target execution.
 * Опубликовать native hint вне shared mutex, без target execution. */
void dvm_launch_shared_changed(LaunchShared *shared);
#endif
