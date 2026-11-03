/* Checked aggregate C tail entries; actual decoded roots/admission stay caller-owned. */
#ifndef DVM_AGGREGATE_FAST_H
#define DVM_AGGREGATE_FAST_H
struct REG;


/* [GC:] Execute actual C movmp once with current full-size ACTIVE REG. Return0
 * before operands on failed shared preflight,1 on completion. VM errors propagate
 * normally, never decline/retry after effects. Caller owns decoded rooted slots,
 * source/Module/Type/frame lifetime and serialized admission; this grants none.
 * Enter by tail JMP, preserving C return PC across cache retirement. */
int dvm_runtime_aggregate_movmp_fast(struct REG *registers);


/* [GC:] Execute actual C headmp once with current full-size ACTIVE REG. Return0
 * before operands on failed shared preflight,1 on completion. VM errors propagate
 * normally, never decline/retry after effects. Caller owns decoded rooted slots,
 * source/Module/Type/frame lifetime and serialized admission; this grants none.
 * Enter by tail JMP, preserving C return PC across cache retirement. */
int dvm_runtime_aggregate_headmp_fast(struct REG *registers);


/* [GC:] Execute actual C consmp once with current full-size ACTIVE REG. Return0
 * before operands on failed shared preflight,1 on completion. VM errors propagate
 * normally, never decline/retry after effects. Caller owns decoded rooted slots,
 * source/Module/Type/frame lifetime and serialized admission; this grants none.
 * Enter by tail JMP, preserving C return PC across cache retirement. */
int dvm_runtime_aggregate_consmp_fast(struct REG *registers);


/* [GC:] Execute actual C consm once with current full-size ACTIVE REG. Return0
 * before operands on failed shared preflight,1 on completion. VM errors propagate
 * normally, never decline/retry after effects. Caller owns decoded rooted slots,
 * source/Module/Type/frame lifetime and serialized admission; this grants none.
 * Enter by tail JMP, preserving C return PC across cache retirement. */
int dvm_runtime_aggregate_consm_fast(struct REG *registers);

#endif
