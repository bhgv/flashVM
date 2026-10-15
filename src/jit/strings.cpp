/* FlashVM: native string bridges reuse counted/versioned AVM1 string services. */
#include "internal.hpp"
#include "../avm1/operand_stack.hpp"
#include "../avm1/operand_constants.h"
namespace
{
/* [GC:] Retain the source until the selected unary service completes collecting coercions. */
template <FVM_Result (*Service)(FVM_Context *, const FVM_Value *, FVM_Value *)>
FVM_Result unary(FVM_JitInvocation *s)
{
    FVM_Result status = fvm_avm1_require_value(s->context->swf_loaded, *s->stack);
    if (status != FVM_OK)
        return status;
    FVM_Value result;
    status = Service(s->context, &s->stack->back(), &result);
    if (status == FVM_OK)
        s->stack->back() = result;
    return status;
}
}
/* Compute the common StringLength/MBStringLength policy. */
FVM_Result fvm_jit_string_length(FVM_JitInvocation *s, unsigned int)
{
    return unary<fvm_exec_string_length>(s);
}
/* Read a character under the common CharToAscii/MBCharToAscii policy. */
FVM_Result fvm_jit_char_code(FVM_JitInvocation *s, unsigned int)
{
    return unary<fvm_exec_char_code>(s);
}
/* Construct an adopted character string under the common AsciiToChar policy. */
FVM_Result fvm_jit_char_from_code(FVM_JitInvocation *s, unsigned int)
{
    return unary<fvm_exec_char_from_code>(s);
}
/* [GC:] Keep both original operands rooted through ordered string conversion and comparison. */
FVM_Result fvm_jit_string_binary(FVM_JitInvocation *s, unsigned int operation)
{
    FVM_Result status = fvm_avm1_require_operands(s->context->swf_loaded, *s->stack,
                                                FVM_AVM1_STRING_BINARY_OPERANDS);
    if (status != FVM_OK)
        return status;
    size_t first = s->stack->size() - FVM_AVM1_STRING_BINARY_OPERANDS;
    FVM_Value result;
    status = operation == FVM_EXEC_ADD_DYNAMIC
        ? fvm_exec_string_add(s->context, &(*s->stack)[first], &s->stack->back(), &result)
        : fvm_exec_string_compare(s->context, (FVM_ExecutionCompare)operation,
                                   &(*s->stack)[first], &s->stack->back(), &result);
    if (status == FVM_OK)
    {
        (*s->stack)[first] = result;
        s->stack->pop_back();
    }
    return status;
}
/* [GC:] Extract only after count, start and input conversion return with all operands rooted. */
FVM_Result fvm_jit_string_extract(FVM_JitInvocation *s, unsigned int)
{
    FVM_Result status = fvm_avm1_require_operands(s->context->swf_loaded, *s->stack,
                                                FVM_AVM1_STRING_EXTRACT_OPERANDS);
    if (status != FVM_OK)
        return status;
    size_t first = s->stack->size() - FVM_AVM1_STRING_EXTRACT_OPERANDS;
    FVM_Value result;
    status = fvm_exec_string_extract(s->context, &(*s->stack)[first],
        &(*s->stack)[s->stack->size() - FVM_AVM1_STRING_EXTRACT_START_OFFSET],
        &s->stack->back(), &result);
    if (status == FVM_OK)
    {
        (*s->stack)[first] = result;
        s->stack->resize(first + 1);
    }
    return status;
}
