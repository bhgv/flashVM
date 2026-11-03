/* Hosted native tail entries; no shared LeafLayout or file-opcode extension. */
#ifndef DVM_OWNER_FAST_H
#define DVM_OWNER_FAST_H
struct REG;


/* Check only native context/REG size/identity and ACTIVE nonpending role.
 * Return1 for matching profile,0 before operands otherwise. This is NOT mapped
 * operand provenance, source lifetime/root acquisition or payload permission. */
int dvm_runtime_owner_entry_ready(struct REG *registers);


/* [GC:] Execute the matching retained C opcode once on this bound ACTIVE REG.
 * Foreign/NULL/inactive/pending callers decline0 BEFORE operand reads. Caller
 * supplies actual decoded rooted slots under serialized managed admission.
 * Return1 after opcode completion; VM errors propagate, never decline/retry
 * after ownership work. Tail entry must preserve original C return PC, with no
 * JIT frame. Tail scaffold never reads JIT/cache state after entering opcode;
 * normal C Module/frame lifetime requirements still apply. This does not acquire
 * admission, source lifetime, roots or payload permission. */
int dvm_runtime_owner_movp_fast(struct REG *registers);
int dvm_runtime_owner_self_fast(struct REG *registers);
int dvm_runtime_owner_headp_fast(struct REG *registers);
int dvm_runtime_owner_tail_fast(struct REG *registers);
#endif
