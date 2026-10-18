/* FlashVM: named-constructor count preparation independent of interpreter operands. */
#include "support.hpp"
/* Preserve the larger strict host bound while sharing loaded call-count conversion. */
extern "C" FVM_Result fvm_exec_constructor_count(FVM_Context *c, const FVM_Value *input,
                                                 size_t available, unsigned int *out)
{
    if (!c || c->destroying || c->gc_releasing || !out || fvm_validate_value(c, input) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    if (c->swf_loaded)
        return fvm_exec_argument_count(c, input, available, out);
    if (input->type != FVM_VALUE_NUMBER ||
        !(input->number >= 0 && input->number <= FVM_EXEC_MAX_CONSTRUCTOR_ARGUMENTS) ||
        input->number != std::floor(input->number) || input->number > available)
        return FVM_ERROR;
    *out = (unsigned int)input->number;
    return FVM_OK;
}
