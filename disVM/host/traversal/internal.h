/* Copyright (c) 2026 bhgv
 * EN: Private scalar traversal entries and thread-local stack ownership.
 * RU: Private scalar entries обхода и thread-local stack ownership.
 */
#ifndef VFS_WALK_INTERNAL_H
#define VFS_WALK_INTERNAL_H
#include "walk.h"
#include <stdint.h>


/* EN/RU: Active borrowed search state; source capability lifetime belongs to its provider call.
 * Active borrowed search state; lifetime source capability принадлежит provider call.
 */
typedef struct WalkEntry
{
    WalkEntity entity; /* Copied exact projection identity. */
    unsigned long high; /* Logical node high32. */
    unsigned long low; /* Logical node low32. */
    unsigned operation; /* Operation kind; not mutable file state. */
    const char *remaining; /* Borrowed canonical search remainder through scope leave. */
    const char *detail; /* Borrowed mode/permission class through scope leave. */
} WalkEntry;


/* EN/RU: Only this native thread accesses entries; root leave releases any grown allocation.
 * Только данный native thread обращается к entries; root leave освобождает grown allocation.
 */
typedef struct WalkStack
{
    WalkEntry small[WALK_INLINE]; /* Allocation-free shallow stack. */
    WalkEntry *entries; /* Small storage or owned grown array. */
    size_t capacity; /* Actual entries extent. */
    size_t count; /* Active states only, never all previously visited nodes. */
} WalkStack;
extern __thread WalkStack walk_stack; /* One search history per native execution thread. */


/* EN/RU: Validate four unsigned32 identity limbs; nonzero required only for exported identities,1/0.
 * Проверить четыре unsigned32 limbs; nonzero только для export,1/0, без side effects.
 */
int walk_words(const unsigned long words[4], int nonzero);
#endif
