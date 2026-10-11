#ifndef FLASHVM_STANDARD_H
#define FLASHVM_STANDARD_H
#include "value.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Install the initial numeric standard library: Math, Number/Boolean conversion,
     * boxed String, Number, Boolean and Error constructors/prototypes, isNaN/isFinite,
     * parseInt/parseFloat, String conversion, NaN and Infinity. This is not the complete AS1/AS2
     * library. Existing bindings are preserved: conflicting names reject installation. */
    FVM_API FVM_Result fvm_standard_install(FVM_Context *);
    /* Deterministic primitive ToNumber, without locale-dependent strtod.
     * Boxed String/Number/Boolean unwrap their primitive values. Generic object valueOf/toString
     * and Unicode whitespace remain unsupported. */
    FVM_API FVM_Result fvm_value_to_number(FVM_Context *, const FVM_Value *, double *);
    /* ToString bridge for primitives and native toString/valueOf callbacks.
     * Output is a context-owned or static string. Script function-valued methods
     * and SWF-version-specific coercion are not implemented. */
    FVM_API FVM_Result fvm_value_to_string(FVM_Context *, const FVM_Value *, FVM_Value *);
    /* Native class/prototype identity bridge for String, Number, Boolean, Error, TextFormat and
     * TextField. Imported graphs retain their own prototype identities. */
    FVM_API FVM_Result fvm_value_instance_of(FVM_Context *, const FVM_Value *,
                                             const FVM_Value *constructor, int *out);
#ifdef __cplusplus
}
#endif
#endif
