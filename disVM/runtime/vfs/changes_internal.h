/* Copyright (c) 2026 bhgv
 * EN: Private bounded journal topology under one per-provider native mutex.
 * RU: Private bounded journal topology под per-provider native mutex.
 */
#ifndef DVM_VFS_CHANGES_INTERNAL_H
#define DVM_VFS_CHANGES_INTERNAL_H
#include "changes.h"
#include "channel_sync.h"


/* EN/RU: No managed references; callbacks never execute while lock is held.
 * Без managed references; callbacks никогда не выполняются под lock. */
struct DVM_Changes
{
    FVM_PortSync *lock; /* Protects journal, current epoch and class versions. */
    uint64_t session; /* Monotonic local connection epoch, initially one. */
    uint64_t generation; /* Highest source generation in the current epoch. */
    uint64_t versions[DVM_CHANGE_KINDS]; /* Last event affecting each class. */
    DVM_ChangeEvent history[DVM_CHANGE_HISTORY]; /* Fixed-size coalescing history. */
    size_t first; /* Oldest retained event's ring index. */
    size_t count; /* Number of valid entries in history. */
    uint64_t floor; /* Last evicted generation; older cursors require broad scope. */
};
#endif
