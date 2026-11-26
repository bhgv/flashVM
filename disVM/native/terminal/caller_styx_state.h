/* Copyright (c) 2026 bhgv
 * EN: Private ownership shared by the reverse Styx dispatcher and its controls.
 * RU: Общее внутреннее владение встречного диспетчера Styx и его управления.
 */
#ifndef STYX_CALLER_STATE_H
#define STYX_CALLER_STATE_H
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "caller_styx.h"
#include "caller_scope.h"
#include "server.h"
#include "tree.h"

#define PROG_QID 1U /* Permanent virtual directory for active process IDs. */

typedef struct CallerNodes
{
    uint64_t generation; /* Publication represented by this dispatcher cache. */
    size_t count; /* Number of visited nodes for this publication. */
    StyxBacking *nodes[STYX_CALLER_NODES]; /* Owned visited nodes. */
} CallerNodes;

struct StyxCallerServer
{
    Styxserver server; /* Owned original Styx dispatcher. */
    StyxPhyWake *wake; /* Owned dispatcher wake pin through mount retirement. */
    pthread_t worker; /* Owns dispatcher, nodes and stat scratch. */
    StyxCallerScopes scopes; /* Concurrent per-PID publications. */
    size_t references; /* Control and bound mount owners, under scopes.lock. */
    CallerNodes cache[STYX_CALLER_SLOTS]; /* Dispatcher-owned node caches. */
    StyxRemoveNode remove; /* Removes visible entries in the mutable shared view. */
    void *remove_state; /* Borrowed state for the remove callback. */
    DVM_Provider *shared; /* Retained optional view with its own access ceiling. */
    StyxBacking *shared_root; /* Identity-bound permanent root for ordinary walks. */
    const char *service_name; /* Borrowed exact root protocol component, or NULL. */
    size_t service_qid; /* Reserved permanent slot, valid with service_name. */
    CallerNodes permanent; /* Ordinary visited nodes in reserved generation zero. */
    StyxMeta stat; /* Borrowed by Rstat encoding until callback completes. */
};

#endif
