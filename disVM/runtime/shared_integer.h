/* Hosted guarded integer conversions; include interp.h before this header. */
#ifndef DVM_SHARED_INTEGER_H
#define DVM_SHARED_INTEGER_H


/* [GC:] Convert admitted CVTBW/WB/WS/SW/WL/LW under one source/output guard.
 * Caller independently owns live storage; this grants no provenance/lifetime.
 * Exact ACTIVE worker context required. Return NULL after publication or a static
 * error before changes. No managed callbacks, allocation or FP work under guards.
 * Same narrowing semantics as retained C; geometry checks precede payload reads. */
const char *dvm_runtime_integer_convert(int opcode, const void *source, void *destination);
#endif
