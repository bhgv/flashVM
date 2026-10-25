/* FlashVM: private scoped-member resolution retaining its actual callable receiver. */
#ifndef FVM_SCOPE_MEMBER_HPP
#define FVM_SCOPE_MEMBER_HPP /* Private member-resolution bridge guard. */
#include "flashvm/execution.h"


/* [GC:] Preserve value/owner/presence outputs on failure; publish the owner found before a getter.
 */
FVM_Result fvm_avm1_read_scope_member(FVM_Context *,
                                      FVM_Object *scope,
                                      FVM_Object *target,
                                      FVM_Object *receiver,
                                      const FVM_Value *path,
                                      const FVM_Value *name,
                                      FVM_Value *out,
                                      FVM_Object **owner,
                                      FVM_Object **source_owner,
                                      int *present);
#endif
