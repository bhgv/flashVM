/* Optional native register binding; upstream structs/member names stay intact. */
#ifndef DISVM_HOST_INTERP_H
#define DISVM_HOST_INTERP_H
#include "../../upstream/include/interp.h"

#ifdef DVM_RUNTIME_GC_FIELDS
#include "gc_fields.h"
#undef Setmark
#define Setmark(h) dvm_runtime_gc_mark(&(h)->color, &mutator, &nprop) /* Protected hosted C scalar barrier; no graph/epoch admission. */
#endif

#ifdef DVM_NATIVE_REGISTERS
#include "disvm/native_context.h"


/* Generated instruction views use DVM_R; process snapshots keep their field R. */
extern REG *dvm_native_registers(void);
#define DVM_R (*dvm_native_registers())
#endif
#endif
