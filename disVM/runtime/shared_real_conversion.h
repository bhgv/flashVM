/* Two-phase scalar FP conversion; lifetime and world admission stay external. */
#ifndef DVM_SHARED_REAL_CONVERSION_H
#define DVM_SHARED_REAL_CONVERSION_H


/* [GC:] opcode is CVTRF/FR/WF/LF/FW/FL. source/destination borrow independently
 * live mapped native cells; unaligned/overlapping spans are permitted. Preflight
 * both extents, capture under source-only exclusion, release, evaluate native FP,
 * then publish under destination-only exclusion. Two observation points, NOT
 * atomic RMW. Return static diagnostic/NULL after unlock; no allocation, root,
 * retain/release, callback or VM error while guarded. Caller retains ACTIVE/world,
 * FP environment, frame/referent/code/GC lifetime. Geometry is not mapping proof. */
const char *dvm_runtime_real_conversion(int opcode, const void *source, void *destination);
#endif
