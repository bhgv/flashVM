/* FlashVM: member semantics for C callers and generated code. */
#include "support.hpp"
/* Execute value semantics with validation and allocation failure containment. */
extern "C" FVM_Result fvm_exec_member(FVM_Context *c, int write, const FVM_Value *owner_value,
                                      const FVM_Value *member_value, const FVM_Value *value,
                                      FVM_Value *out)
{
    if (!c || !out || fvm_validate_value(c, owner_value) != FVM_OK ||
        fvm_validate_value(c, member_value) != FVM_OK ||
        (write && fvm_validate_value(c, value) != FVM_OK))
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_ExecutionRoots roots(c, 4);
        roots.values[0] = *owner_value;
        roots.values[1] = *member_value;
        if (write)
            roots.values[2] = *value;
        const FVM_Value &owner = roots.values[0], &member = roots.values[1];
        FVM_Value result;
        fvm_value_undefined(&result);
        FVM_Result status;
        if (owner.type == FVM_VALUE_OBJECT &&
            owner.object->resource_kind == FVM_RESOURCE_PLAIN_OBJECT && owner.object != c->global &&
            member.type == FVM_VALUE_NUMBER && member.number >= 0 &&
            member.number <= FVM_UINT32_MAX_NUMBER && member.number == std::floor(member.number))
        {
            if (write)
                status = fvm_table_set_index(c, owner.object, (unsigned long)member.number,
                                             &roots.values[2]);
            else
                status =
                    fvm_table_get_index(c, owner.object, (unsigned long)member.number, &result);
        }
        else
        {
            status = fvm_avm1_string(c, member, &roots.values[3]);
            const FVM_Value &key = roots.values[3];
            if (status == FVM_OK)
            {
                if (write)
                    status = fvm_value_set_property_bytes(c, &owner, key.string,
                                                          fvm_string_size(key), &roots.values[2]);
                else
                    status = fvm_value_get_property_bytes(c, &owner, key.string,
                                                          fvm_string_size(key), &result);
            }
        }
        if (status == FVM_OK)
            *out = result;
        return status;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
