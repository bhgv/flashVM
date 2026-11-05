/* Hosted pointer-slot ownership copy; caller includes native interp.h. */
#ifndef DVM_SHARED_POINTER_H
#define DVM_SHARED_POINTER_H


/* [GC:] Copy one already-owned source cell into destination while preserving
 * an independent displaced owner in private disjoint previous. Return NULL on
 * success or a borrowed static exception before publication on refusal. Caller
 * owns mapped cell spans/root lifetimes and exact native VM role. Active worker
 * worlds use ordered range guards; detached inline mode remains serial. Never
 * calls VM/throws/destroys under guards. Caller must error on refusal or destroy
 * *previous after success, with no decline/retry after ownership effects. */
const char *dvm_runtime_movp_copy(const void *source, void *destination,
    void **previous);


/* Borrow the same cells as movp_copy. A scheduled replay sets replay and
 * leaves previous untouched. Callers retire a previous owner only on commit. */
const char *dvm_runtime_movp_opcode(int opcode, const void *source, void *destination,
    void **previous, int *replay);
#endif
