/* Copyright (c) 2026 bhgv
 * EN: Request-local active VFS traversal states, independent of VM heaps and transport carriers.
 * RU: Request-local active states обхода VFS, независимо от VM heaps и transport carriers.
 */
#ifndef VFS_WALK_H
#define VFS_WALK_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#define WALK_INLINE 8U /* Shallow native traversals allocate no storage. */
#define WALK_DEPTH_LIMIT 128U /* Separate finite work bound, never reported as a proven cycle. */
#define WALK_PATH_LIMIT 4096U /* Bytes including terminating NUL in remaining path. */
#define WALK_DETAIL_LIMIT 32U /* Bytes including NUL in operation-specific mode. */
#define WALK_WIRE_LIMIT (4U + WALK_DEPTH_LIMIT * (44U + WALK_PATH_LIMIT + WALK_DETAIL_LIMIT)) /* Maximum copied history bytes. */
#define WALK_INFO 1U /* Path metadata lookup. */
#define WALK_OPEN 2U /* Open a named resource. */
#define WALK_ACCESS 3U /* Effective permission query. */
#define WALK_CAPTURE 4U /* Capture a stable subtree/node capability. */
#define WALK_LAUNCH 5U /* Discover an owner-local launch endpoint. */
#define WALK_LIST 6U /* Enumerate an opened directory. */
#define WALK_FSTAT 7U /* Metadata of an already opened object. */
#define WALK_OK 1 /* Scope entered; caller must leave it. */
#define WALK_CYCLE 0 /* Exact active state repeated; no scope was entered. */
#define WALK_INVALID (-1) /* Invalid identity/state/scope. */
#define WALK_LIMIT (-2) /* Work/depth bound, distinct from a detected cycle. */
#define WALK_NOMEM (-3) /* Storage allocation failed without changing active history. */


/* EN/RU: Trusted logical identity; values are unsigned32 limbs, never pointer bytes or mutable permissions.
 * Trusted logical identity; unsigned32 limbs, не pointer bytes и не mutable permissions.
 */
typedef struct WalkEntity
{
    unsigned long domain[4]; /* Connection/VM identity namespace; zero is allowed only for local unexported history. */
    unsigned long object[4]; /* Exact granted projection/root identity within that namespace. */
} WalkEntity;


/* EN/RU: Caller-owned zeroed scope; one native thread owns it through leave, no copies while active.
 * Caller-owned нулевой scope; один native thread до leave, active scope не копировать.
 */
typedef struct WalkScope
{
    const void *owner; /* Local native thread identity only, never part of a serialized entry. */
    size_t mark; /* Active stack depth before entry/import. */
    size_t entered; /* Required balanced depth when leaving. */
    void *owned; /* Optional imported scalar snapshot storage, no VM/native object pointers. */
    int active; /* Exactly one successful enter/import awaits leave. */
} WalkScope;


/* EN/RU: Current thread's cycle observation serial, no allocation/I/O or scope mutation; diagnostic only.
 * Serial циклов current thread, без allocation/I/O/scope mutation; только diagnostic.
 */
unsigned long walk_cycles(void);


/* EN/RU: Record one proven local/remote cycle on the actual native caller, without creating an active entry.
 * Отметить proven local/remote cycle на actual native caller, не создавая active entry.
 */
void walk_note_cycle(void);


/* EN/RU: Set one nonzero unsigned32-limb VM identity before remote history use;1 same/set,0 invalid/conflicting/active.
 * Domain is descriptive, not authority; caller supplies stable unique identity. No allocation/I/O.
 * Установить nonzero VM identity до remote history;1 same/set,0 invalid/conflicting/active. Без allocation/I/O.
 */
int walk_domain(const unsigned long domain[4]);


/* EN/RU: Copy configured domain under its native publication mutex;1/0 unset/NULL, no I/O or authority.
 * Скопировать configured domain под native mutex;1/0 unset/NULL, без I/O или authority.
 */
int walk_domain_get(unsigned long output[4]);


/* EN/RU: Mint native root identity into borrowed output;1/0 on exhausted identity counter, no allocation/I/O.
 * Current configured domain is copied; zero domain remains local-only until caller supplies an explicit identity.
 * Создать native root identity в output;1/0 при exhausted counter, без allocation/I/O; zero domain — local-only.
 */
int walk_entity(WalkEntity *output);


/* EN/RU: Push exact entity/node/operation/remaining/detail state; return WALK_* and enter only on OK.
 * Borrow strings/entity through leave; node is an unsigned32-limb object identifier, not a bind occurrence.
 * Top-level invocation owns a new search; parallel native threads have independent histories.
 * Push exact entity/node/operation/remaining/detail; WALK_*, scope возникает лишь при OK.
 * Strings/entity borrowed до leave; node — unsigned32 limbs, не occurrence bind; parallel histories независимы.
 */
int walk_enter(WalkScope *scope, const WalkEntity *entity, unsigned long node_high, unsigned long node_low,
    unsigned operation, const char *remaining, const char *detail);


/* EN/RU: Pop exactly this active scope and release imported/deep storage;1/0 on unbalanced/wrong-thread use.
 * A failed enter must not be left. No callbacks, I/O or managed owners.
 * Pop active scope, освободить imported/deep storage;1/0 при unbalanced/wrong-thread use. Без callbacks/I/O.
 */
int walk_leave(WalkScope *scope);


/* EN/RU: Current native thread's active depth,0 between operations; no allocation or side effects.
 * Active depth текущего native thread;0 между operations, без allocation/side effects.
 */
size_t walk_depth(void);


/* EN/RU: Exact exportable current history size including header;0 if any entry has a local-only domain.
 * Native owner only, no allocation/I/O. Точный размер exportable current history;0 при local-only domain.
 */
size_t walk_export_size(void);


/* EN/RU: Encode active scalar history into caller bytes;1 success/0 invalid/capacity/local-only identity.
 * used receives exact extent only on success. No pointers/layout serialized; empty history is valid.
 * Кодировать active scalar history в bytes;1/0 invalid/capacity/local-only, used меняется лишь при success.
 */
int walk_export(void *bytes, size_t capacity, size_t *used);


/* EN/RU: Append a validated copied remote history to this native thread; WALK_*; owned scope only on OK.
 * Borrow bytes through return only. Reject cycles/malformed/bounds before any provider callback, unwind on failure.
 * Добавить validated copied remote history; WALK_*, scope лишь при OK; bytes borrowed до return.
 * Cycles/malformed/bounds отказать до provider callback; rollback при failure.
 */
int walk_import(WalkScope *scope, const void *bytes, size_t size);
#ifdef __cplusplus
}
#endif
#endif
