/* FlashVM: Public embedding API declarations: meta call.
 * File: include/flashvm/meta_call.h.
 */

#ifndef FLASHVM_META_CALL_H
/* Define FLASHVM META CALL H constant or compilation switch. */
#define FLASHVM_META_CALL_H
#include "value.h"
#ifdef __cplusplus
extern "C"
{
#endif
/* Invoke the owner's __call metamethod. Callback arguments are:
     * 0: owner object, 1: defining metatable, 2: fresh indexed argument table.
     * Index 0 holds the first user argument. The argument table belongs to the
     * context. Native callbacks may use fvm_table_get_index to inspect it.
     * No implicit bytecode invocation or script closure dispatch is provided.
     */


/* Call meta. */
FVM_API FVM_Result fvm_meta_call(FVM_Context *, FVM_Object *, unsigned int argc,
                                 const FVM_Value *argv, FVM_Value *result);
#ifdef __cplusplus
}
#endif
#endif
