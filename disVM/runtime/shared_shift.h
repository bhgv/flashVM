/* Hosted mixed-width guarded shifts; include interp.h first. */
#ifndef DVM_SHARED_SHIFT_H
#define DVM_SHARED_SHIFT_H


/* [GC:] opcode selects eight native shift variants. first/output are borrowed
 * value cells, second a borrowed WORD count cell. Require exact ACTIVE owner.
 * Return NULL after publication or static error after unlock. Invalid count wins
 * over nil/H/wrapped outer geometry; native admission refusal precedes semantics.
 * Caller owns mapped storage/root lifetime. No allocation or VM callback under
 * guards, no mapping/provenance or general parallel admission is granted. */
const char *dvm_runtime_shift(int opcode, const void *first, const void *second, void *output);
#endif
