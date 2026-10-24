/* Optional native register binding; upstream structs/member names stay intact. */
#ifndef DISVM_HOST_INTERP_H
#define DISVM_HOST_INTERP_H
#include "../../upstream/include/interp.h"
#ifdef DVM_NATIVE_REGISTERS
/* Generated instruction views use DVM_R; process snapshots keep their field R. */
extern REG *dvm_native_registers(void);
#define DVM_R (*dvm_native_registers())
#endif
#endif
