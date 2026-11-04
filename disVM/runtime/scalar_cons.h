/* Shared scalar constructor boundary; no serialized ABI. */
#ifndef DVM_SCALAR_CONS_H
#define DVM_SCALAR_CONS_H
#include <stddef.h>


/* [GC:] source/destination are live mapped scalar/owning-list operand cells in
 * rooted frames/containers. width is1/4/8, extent >= width fits private24 bytes.
 * Capture once under source span guard, prepare raw node, conditional commit,
 * rebuild stale tails at most8 times, drain all holds preserving first error.
 * Return0 before operand reads for unselected/serial-inline mode; return1 only
 * after completed worker commit/cleanup. Errors never decline after side effects.
 * Require ACTIVE/NativeWorld, stable frame/Type/code/cell lifetime and no guards.
 * Callback may inspect/raise or perform separate compliant destination writes,
 * never retain private nodes/holds or leave admission. Cleanup error after commit
 * does not undo publication. Not general parallel allocator/collector permission. */
int dvm_runtime_scalar_cons(void *source, void *destination, size_t width, size_t extent);

struct REG;


/* [GC:] registers is bound full-size ACTIVE REG with published decoded roots.
 * Checked C tail targets for actual1/4/8-byte scalar profiles; byte extent uses
 * IBY2WD. Return0 before operands on foreign/inactive/guarded context,1 after
 * actual coordinator completion. VM errors propagate across ordinary C stack,
 * never a retired JIT return PC. Caller preserves cells/Type/frame/code lifetime. */
int dvm_runtime_scalar_1_fast(struct REG *registers);
int dvm_runtime_scalar_4_fast(struct REG *registers);
int dvm_runtime_scalar_8_fast(struct REG *registers);
#endif
