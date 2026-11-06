/* Diagnostic boundary for the retained hosted Inferno exception walker. */
#ifndef DVM_EXCEPTION_HOST_H
#define DVM_EXCEPTION_HOST_H



/* Copy the current bound owner's module path and predecessor PC for diagnostics.
 * registers must be its active REG; name is borrowed writable capacity bytes.
 * No heap allocation, ownership transfer or payload lock acquisition occurs. */
WORD dvm_exception_status(REG *registers, char *name, int capacity);
struct Prog;
/* Start an audited read-only handler lookup. */
int dvm_runtime_exception_search_enter(struct Prog *process);
/* Pop an ordinary frame through the shared retirement protocol. */
int dvm_runtime_exception_pop(struct Prog *process, const unsigned char *handler_frame);

/* Publish an already-owned exception and defer displaced handler slots. */
int dvm_runtime_exception_install(Prog *process, Type *cleanup, void *slot, Inst *target);

/* Transfer validated pending argument frames and return the remaining suffix. */
unsigned char *dvm_runtime_exception_pending(struct Prog *process, unsigned char *last);

#endif
