/* FlashVM: internal AVM1 action blocks bounded by SWF tag/frame record lengths. */
#ifndef FVM_AVM1_BLOCK_HPP
#define FVM_AVM1_BLOCK_HPP /* Guard for private bounded-block entry points. */
#include "flashvm/avm1.h"
/* Validate complete records; an explicit ActionEnd is optional at the exact block boundary. */
FVM_Result fvm_avm1_inspect_block(const void *, size_t, FVM_AVM1Info *);
/* Execute a stored frame block, including empty/implicit-end blocks, with its retained receiver. */
FVM_Result fvm_avm1_execute_block_receiver(FVM_Context *, const void *, size_t, FVM_Object *);
#endif
