/* Guarded unmapped-record list constructor, no serialized or guest ABI. */
#ifndef DVM_RAW_CONS_H
#define DVM_RAW_CONS_H


/* [GC:] source/destination/size_cell are independently mapped live operands in
 * rooted frames/containers; size_cell holds a native WORD byte extent. Source
 * contains NO managed references requiring tracing/retention. Zero size ignores
 * source. Capture size and source once under their span/range guards into native
 * storage before callbacks; then held tail, rooted preparation, conditional
 * publication and at most8 stale rebuilds. Operands may overlap. All temporary
 * holders and native snapshot drain despite cleanup errors; first error wins.
 * Return0 before operands in unselected/serial-inline mode;1 after successful
 * worker commit/cleanup. Error never declines after effects; postcommit cleanup
 * error does not undo publication. Caller preserves Type/frame/code/operand/root
 * lifetime under ACTIVE/NativeWorld, no object guards, ordinary GC exclusion.
 * Monitor may inspect/raise or perform separate compliant destination writes,
 * not retain/mutate private state or leave admission. Not mapped-record or
 * general parallel allocator/collector permission. */
int dvm_runtime_raw_cons(void *source, void *destination, void *size_cell);
#endif
