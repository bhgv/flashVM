/* Internal ownership-proof consumers; public fact storage is declared in fir.h. */
#ifndef FIR_LOCALITY_H
#define FIR_LOCALITY_H
#include "fir.h"


/* Return nonzero when op may publish/reenter unknown code. Typed table builtins
 * have a reviewed nonpublishing contract, independently of broad DCE effects.
 * Unknown opcodes stay barriers. No runtime state, allocation or callbacks. */
int fir_locality_barrier(FirOp op);


/* Rewrite proved private, retained, same-block numeric store/load pairs. module
 * must be verified; return nonzero if changed. Preserve every lock/store/error
 * producer; no allocation, VM execution or runtime local/shared test is added. */
int fir_forward_private_tables(FirModule *module);
#endif
