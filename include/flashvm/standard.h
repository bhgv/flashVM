#ifndef FLASHVM_STANDARD_H
#define FLASHVM_STANDARD_H
#include "value.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Install the initial numeric standard library: Math, Number/Boolean conversion,
     * boxed Boolean constructor/prototype, isNaN/isFinite, NaN and Infinity.
     * This is not the complete AS1/AS2 library.
     * Existing bindings are preserved: conflicting names reject installation. */
    FVM_API FVM_Result fvm_standard_install(FVM_Context *);
    /* Deterministic primitive ToNumber, without locale-dependent strtod.
     * Boxed Boolean unwraps its primitive. Generic object valueOf/toString and
     * Unicode whitespace remain unsupported. */
    FVM_API FVM_Result fvm_value_to_number(FVM_Context *, const FVM_Value *, double *);
#ifdef __cplusplus
}
#endif
#endif
