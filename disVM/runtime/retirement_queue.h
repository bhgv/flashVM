/* Native retirement metadata; managed ownership/root policy belongs to caller. */
#ifndef DVM_RETIREMENT_QUEUE_H
#define DVM_RETIREMENT_QUEUE_H
#include "disvm/heap_settings.h"

typedef struct DVM_RetirementQueue DVM_RetirementQueue; /* Stable opaque native owner. */


/* Private, noncopyable reservation. Zero initialize before reserve; consume
 * exactly once by commit/cancel before queue lifetime ends. Not a managed pin. */
typedef struct DVM_RetirementReservation
{
    DVM_RetirementQueue *queue; /* Borrowed owner identity through consumption. */
    unsigned long generation; /* Nonwrapping reservation identity. */
    unsigned int slot; /* Native slot, not a serialized pointer/index. */
} DVM_RetirementReservation;


/* Coherent metadata snapshot, never a graph/owner observation. */
typedef struct DVM_RetirementStats
{
    unsigned int ready; /* Committed entries available to take. */
    unsigned int reserved; /* Unpublished entries still owned by producers. */
    unsigned long bytes; /* Ready plus reserved direct extents. */
    int accepting; /* New reservations allowed; existing ones can still finish. */
    unsigned int capacity; /* Current native slot count. */
    unsigned int peak_capacity; /* Largest native slot count since creation. */
} DVM_RetirementStats;


/* Reason for reservation refusal; all refusals leave queue/token unchanged. */
typedef enum DVM_RetirementResult
{
    DVM_RETIRE_OK, /* Slot and byte budget reserved, value not yet published. */
    DVM_RETIRE_CLOSED, /* No new work after close. */
    DVM_RETIRE_FULL, /* No free slot and growth unavailable. */
    DVM_RETIRE_BYTES, /* Direct-byte budget exhausted. */
    DVM_RETIRE_INVALID, /* Invalid queue or occupied reservation. */
    DVM_RETIRE_EXHAUSTED /* Nonwrapping generation counter exhausted. */
} DVM_RetirementResult;


/* Create private native queue with capacity slots and byte_limit direct bytes.
 * Return NULL on invalid capacity/allocation failure. Publish externally only
 * after success. No callbacks, managed pointers or locks escape initialization. */
DVM_RetirementQueue *dvm_retirement_create(unsigned int capacity, unsigned long byte_limit);
DVM_RetirementQueue *dvm_retirement_create_growing(unsigned int capacity);
DVM_RetirementQueue *dvm_retirement_create_indexed(unsigned int capacity, unsigned long byte_limit);


/* As indexed creation, with doubling on slot exhaustion up to a native
 * metadata budget of byte_limit or the initial allocation, whichever is larger.
 * Returns to its initial size when empty. Allocation occurs outside the mutex. */
DVM_RetirementQueue *dvm_retirement_create_growing_indexed(unsigned int capacity,
    unsigned long byte_limit);
int dvm_retirement_commit_key(DVM_RetirementQueue *queue,
    DVM_RetirementReservation *token, void *value, DVM_HeapKey key, int *became_ready);
int dvm_retirement_contains_key(DVM_RetirementQueue *queue, DVM_HeapKey key);



/* Reserve metadata before producer performs fallible root setup outside lock.
 * queue/token must remain live; token is exclusively caller-owned and empty.
 * May wait for native mutex, never for capacity. No allocation/callback/VM call. */
DVM_RetirementResult dvm_retirement_reserve(DVM_RetirementQueue *queue,
    unsigned long bytes, DVM_RetirementReservation *token);


/* Publish borrowed value identity and consume valid token; return1 on success.
 * Caller transfers its independently protected owner/root, not acquired here.
 * NULL value/foreign/stale token returns0 unchanged. Commit is allowed after
 * close for an already accepted reservation. No allocation/callback/destructor. */
int dvm_retirement_commit(DVM_RetirementQueue *queue,
    DVM_RetirementReservation *token, void *value);


/* As commit, with an optional output set on success iff the ready queue
 * changed from empty to nonempty. A refused commit leaves it untouched. */
int dvm_retirement_commit_ready(DVM_RetirementQueue *queue,
    DVM_RetirementReservation *token, void *value, int *became_ready);


/* Cancel valid unpublished token, returning its slot/byte budget. Return0
 * unchanged for invalid token. Does not release any caller object/root. */
int dvm_retirement_cancel(DVM_RetirementQueue *queue, DVM_RetirementReservation *token);


/* Borrow head identity until the sole consumer takes it. */
void *dvm_retirement_peek(DVM_RetirementQueue *queue);

/* Move the head to the tail, even at capacity. Single consumer only.
 * The queue must be live. An empty or singleton queue is unchanged. */
void dvm_retirement_rotate(DVM_RetirementQueue *queue);

/* Move oldest committed identity into *value and its extent into *bytes.
 * Output pointers are private, nonaliasing and live; unchanged on empty/invalid.
 * Return1 if moved. Caller handles root/destruction only AFTER this returns.
 * No object read, retain, callback or destruction occurs under metadata lock. */
int dvm_retirement_take(DVM_RetirementQueue *queue, void **value, unsigned long *bytes);


/* Close new admission, retaining pending reservations and committed entries.
 * Caller must ensure every producer finishes and every entry retires before
 * destroy. Does not join/wait for producers or cancel their ownership. */
void dvm_retirement_close(DVM_RetirementQueue *queue);


/* Fill private snapshot under mutex; return0 unchanged on NULL arguments.
 * Snapshot is not a reservation, pin, drain completion or payload permission. */
int dvm_retirement_statistics(DVM_RetirementQueue *queue, DVM_RetirementStats *stats);


/* Destroy EMPTY, quiescent queue and clear *queue. Return0 unchanged while any
 * ready/reserved slot remains. Caller independently joins/excludes every user;
 * mutex cannot protect its own lifetime. Native resources free after unlock. */
int dvm_retirement_destroy(DVM_RetirementQueue **queue);
#endif
