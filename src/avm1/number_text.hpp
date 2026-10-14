/* FlashVM: loaded AVM1 decimal Number spelling, separate from host roundtrip formatting. */
#ifndef FVM_AVM1_NUMBER_TEXT_HPP
#define FVM_AVM1_NUMBER_TEXT_HPP /* Private numeric-text guard. */
#include "flashvm/value.h"
/* [GC:] Format the scalar and retain its counted text in the context's string storage. */
FVM_Result fvm_avm1_number_text(FVM_Context *, double, FVM_Value *);
#endif
