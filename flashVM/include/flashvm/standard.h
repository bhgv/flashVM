/* FlashVM: Public embedding API declarations: standard.
 * File: include/flashvm/standard.h.
 */

#ifndef FLASHVM_STANDARD_H
/* Define FLASHVM STANDARD H constant or compilation switch. */
#define FLASHVM_STANDARD_H
#include "value.h"
#ifdef __cplusplus
extern "C"
{
#endif
/* Install the initial numeric standard library: Math, Number/Boolean conversion,
     * boxed String, Number, Boolean and Error constructors/prototypes, isNaN/isFinite,
     * parseInt/parseFloat, escape/unescape (SWF5+), String conversion, getTimer (SWF4+), NaN and Infinity.
     * Timers: setInterval/clearInterval (SWF6+), setTimeout/clearTimeout (SWF8+); poll via timers.h.
     * updateAfterEvent (SWF5+) coalesces redraw requests in supported event/interval call chains.
     * Partial AS1/AS2 library; conflicting own names follow active SWF case rules and are preserved. */


/* Implement standard install for this subsystem. */
FVM_API FVM_Result fvm_standard_install(FVM_Context *);
/* Deterministic primitive ToNumber, without locale-dependent strtod.
     * Boxed String/Number/Boolean unwrap their primitive values. Generic object valueOf/toString
     * and Unicode whitespace remain unsupported. */


/* Implement value to number for this subsystem. */
FVM_API FVM_Result fvm_value_to_number(FVM_Context *, const FVM_Value *, double *);
/* ToString bridge for primitives and native toString/valueOf callbacks.
     * Output is a context-owned or static string. Script function-valued methods
     * and SWF-version-specific coercion are not implemented. */


/* Implement value to string for this subsystem. */
FVM_API FVM_Result fvm_value_to_string(FVM_Context *, const FVM_Value *, FVM_Value *);
/* Native class/prototype identity bridge for String, Number, Boolean, Error, TextFormat and
     * TextField. Imported graphs retain their own prototype identities. */


/* Implement value instance of for this subsystem. */
FVM_API FVM_Result fvm_value_instance_of(FVM_Context *, const FVM_Value *,
                                         const FVM_Value *constructor, int *out);
/* Borrowed constructor registered by Object.registerClass; NOT_FOUND if absent. */


/* Implement registered class for this subsystem. */
FVM_API FVM_Result fvm_registered_class(FVM_Context *, const char *, FVM_Object **);


/* Resolve a complete counted UTF-16 symbol key; returned constructor is borrowed. */
FVM_API FVM_Result fvm_registered_class_bytes(FVM_Context *, const char *, size_t, FVM_Object **);
#ifdef __cplusplus
}
#endif
#endif
