/* FlashVM: shared counted AVM1 decimal accumulation and optional native work charging. */
#ifndef FVM_AVM1_DECIMAL_HPP
#define FVM_AVM1_DECIMAL_HPP /* Guard for the private C++98 numeric parser. */
#include "flashvm/value.h"
class FVM_ExecutionWorkScope; /* Optional caller-owned traversal quota; no callback ownership. */
/* Parse validated counted WTF-8. Strict mode rejects trailing grammar; invalid input
 * yields NaN. A null work scope retains the existing uncharged numeric-coercion policy. */
FVM_Result fvm_avm1_decimal(const char *, size_t, bool, double *, FVM_ExecutionWorkScope * = 0);
#endif
