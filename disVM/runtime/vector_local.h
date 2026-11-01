/* Copied path modifiers on the FlashVM owner; no Dis/AVM heap pointers. */
#ifndef DVM_VECTOR_LOCAL_H
#define DVM_VECTOR_LOCAL_H

#define DVM_VECTOR_LOCAL_NODES 4096U /* Explicit source/overlay command bound. */


/* Same scalar coordinates as the private copied vector stream. */
typedef struct DVM_VectorCommand
{
    unsigned long verb; /* Move0, line1, quadratic2, close3, cubic4. */
    unsigned long x; /* Destination X,0..1000. */
    unsigned long y; /* Destination Y,0..600. */
    unsigned long cx; /* First curve control X. */
    unsigned long cy; /* First curve control Y. */
    unsigned long c2x; /* Second cubic control X. */
    unsigned long c2y; /* Second cubic control Y. */
} DVM_VectorCommand;


/* Value plan supplied by the document/UI policy owner, never inferred from paint. */
typedef struct DVM_VectorLocalPlan
{
    unsigned long valid; /* One only after complete validation. */
    unsigned long source; /* Zero-based source path index. */
    unsigned long revision; /* Exact owner source generation. */
    unsigned long x; /* Pointer X represented by the supplied baseline geometry. */
    unsigned long y; /* Pointer Y represented by the supplied baseline geometry. */
    unsigned long count; /* Source command count. */
    unsigned long overlay; /* Overlay command count, including source prefix. */
    unsigned long masks[DVM_VECTOR_LOCAL_NODES]; /* Three source-coordinate selection bits per node. */
    unsigned long bindings[DVM_VECTOR_LOCAL_NODES]; /* One-based point<<8 plus copied marker modifier. */
} DVM_VectorLocalPlan;

typedef struct DVM_VectorLocal DVM_VectorLocal; /* Owned detached gesture baseline. */


/* Validate borrowed source/overlay against the copied plan. Return0 on any bad
 * verb, extent, mask, binding or prefix; no input/storage is mutated. */
int dvm_vector_local_valid(const DVM_VectorLocalPlan *, const DVM_VectorCommand *, const DVM_VectorCommand *);


/* Copy one valid baseline and plan; caller owns the returned handle. Null means
 * invalid input/OOM. No VM, I/O, callbacks or worker references are retained. */
DVM_VectorLocal *dvm_vector_local_create(const DVM_VectorLocalPlan *, const DVM_VectorCommand *, const DVM_VectorCommand *);


/* Return whether a newly decoded canonical plan/geometry is the same baseline.
 * Used before reapplying an unacknowledged native motion after unrelated views. */
int dvm_vector_local_matches(const DVM_VectorLocal *, const DVM_VectorLocalPlan *, const DVM_VectorCommand *, const DVM_VectorCommand *);


/* Transform from the immutable baseline into caller-owned output arrays. Return
 * 0 for out-of-page/refused motion without changing outputs;1 for success. The
 * latest valid pointer is retained for release commit after an invalid endpoint. */
int dvm_vector_local_move(DVM_VectorLocal *, long, long, DVM_VectorCommand *, DVM_VectorCommand *);


/* Copy the latest valid pointer to borrowed outputs. No side effects. */
void dvm_vector_local_point(const DVM_VectorLocal *, long *, long *);


/* Restore caller outputs from the immutable baseline; owner-only, no allocation. */
void dvm_vector_local_restore(const DVM_VectorLocal *, DVM_VectorCommand *, DVM_VectorCommand *);


/* Release an owned handle; null is harmless. No managed objects are involved. */
void dvm_vector_local_destroy(DVM_VectorLocal *);
#endif
