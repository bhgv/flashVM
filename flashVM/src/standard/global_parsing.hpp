/* FlashVM: Built-in ActionScript objects: global parsing.
 * File: src/standard/global_parsing.hpp.
 */

#ifndef FVM_GLOBAL_PARSING_HPP
/* Define FVM GLOBAL PARSING HPP constant or compilation switch. */
#define FVM_GLOBAL_PARSING_HPP
#include "flashvm/value.h"


/* Parse builtin int. */
FVM_Result fvm_builtin_parse_int(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *, void *);


/* Parse builtin float. */
FVM_Result fvm_builtin_parse_float(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *, void *);


/* Implement builtin string for this subsystem. */
FVM_Result fvm_builtin_string(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *, void *);
#endif
