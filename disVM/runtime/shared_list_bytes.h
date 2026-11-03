/* Internal guarded scalar/raw list reads, not typed reference assignment. */
#ifndef DVM_SHARED_LIST_BYTES_H
#define DVM_SHARED_LIST_BYTES_H


/* [GC:] opcode is one of HEADB/W/L/F/M. Capture current REG operands once under
 * ACTIVE admission, retain/root the source list before interior access, validate
 * destination/count/actual pool extent and copy through the shared byte domain.
 * Scalar destination validation comes first; HEADM preserves list/count/empty
 * precedence. Errors finish the source hold before propagation. Caller supplies
 * actual live source/result/length storage and correct raw-vs-reference typing.
 * No new provenance, GC-epoch or parallel admission capability. Return void. */
void dvm_runtime_list_bytes_copy(int opcode);
#endif
