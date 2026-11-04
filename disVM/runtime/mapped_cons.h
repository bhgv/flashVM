/* Typed list-constructor coordinator; no serialized or guest ABI. */
#ifndef DVM_MAPPED_CONS_H
#define DVM_MAPPED_CONS_H
struct Type;


/* [GC:] source/destination are genuinely mapped owning record/list cells in
 * rooted frames/containers, type is validated live immutable module descriptor.
 * Require ACTIVE/NativeWorld, no object guards, stable Type/frame/code/cell roots.
 * First capture a traced immutable head, then build/commit candidates against
 * current held tail, at most8 stale rebuilds. Complete five-lease drain preserves
 * first body/cleanup error; queued Fast retirement may remain. Postcommit error
 * does not undo publication. Return0 before operands for unselected/serial-inline
 * mode,1 only after actual worker commit/cleanup. Never decline after effects.
 * Monitor sees a complete rooted node per snapshot/candidate allocation and may
 * inspect/raise or do separate compliant application writes, not retain/mutate
 * private nodes/holds or leave admission. No general parallel GC/allocation grant. */
int dvm_runtime_mapped_cons(void *source, void *destination, struct Type *type);
#endif
