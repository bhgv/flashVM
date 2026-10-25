/* FlashVM: Public embedding API declarations: callable.
 * File: include/flashvm/callable.h.
 */

#ifndef FLASHVM_CALLABLE_H
/* Define FLASHVM CALLABLE H constant or compilation switch. */
#define FLASHVM_CALLABLE_H
#include "stack.h"
#ifdef __cplusplus
extern "C"
{
#endif
/* Context-owned callable objects: no cross-context transfer or concurrent access.
     * This bridge supports registered C stack callbacks. Script closures are future work.
     * A callable can be stored in any object property as an ordinary object value.
     * The binding lives until the callable object is reclaimed.
     */


/* Create callable. */
FVM_API FVM_Result fvm_callable_create(FVM_Context *, FVM_StackFunction, void *userdata,
                                       FVM_Object **result);


/* Create callable rooted. */
FVM_API FVM_Result fvm_callable_create_rooted(FVM_Context *, FVM_StackFunction, void *,
                                              FVM_Object *capture, FVM_Object **);


/* Test whether callable. */
FVM_API FVM_Result fvm_callable_is(FVM_Context *, FVM_Object *, int *result);


/* Invoke callable. */
FVM_API FVM_Result fvm_callable_invoke(FVM_Context *, FVM_Object *, unsigned int argc,
                                       const FVM_Value *argv, FVM_Value *result);
#ifdef __cplusplus
}
#endif
#endif
