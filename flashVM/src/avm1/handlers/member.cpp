/* FlashVM: member semantics for C callers and generated code. */
#include "support.hpp"
#include "member_constants.h"


/* Execute value semantics with validation and allocation failure containment. */
extern "C" FVM_Result fvm_exec_member(FVM_Context *c,
                                      int write,
                                      const FVM_Value *owner_value,
                                      const FVM_Value *member_value,
                                      const FVM_Value *value,
                                      FVM_Value *out)
{
    if (!c || !out || fvm_validate_value(c, owner_value) != FVM_OK || fvm_validate_value(c, member_value) != FVM_OK ||
        (write && fvm_validate_value(c, value) != FVM_OK))
    {
        return FVM_INVALID_ARGUMENT;
    }
    try
    {


        FVM_ExecutionRoots roots(c, FVM_MEMBER_ROOT_COUNT);
        roots.values[FVM_MEMBER_OWNER_ROOT] = *owner_value;
        roots.values[FVM_MEMBER_NAME_ROOT] = *member_value;
        if (write)
        {
            roots.values[FVM_MEMBER_ASSIGNED_ROOT] = *value;
        }
        const FVM_Value &owner = roots.values[FVM_MEMBER_OWNER_ROOT], &member = roots.values[FVM_MEMBER_NAME_ROOT];
        FVM_Value result;
        fvm_value_undefined(&result);
        FVM_Result status;
        if (owner.type == FVM_VALUE_OBJECT && owner.object->resource_kind == FVM_RESOURCE_PLAIN_OBJECT &&
            owner.object != c->global && member.type == FVM_VALUE_NUMBER && member.number >= 0 &&
            member.number <= FVM_UINT32_MAX_NUMBER && member.number == std::floor(member.number))
        {
            if (write)
            {
                status = fvm_table_set_index(
                    c, owner.object, (unsigned long)member.number, &roots.values[FVM_MEMBER_ASSIGNED_ROOT]);
            }
            else
            {
                status = fvm_table_get_index(c, owner.object, (unsigned long)member.number, &result);
            }
        }
        else
        {
            status = fvm_avm1_string(c, member, &roots.values[FVM_MEMBER_TEXT_ROOT]);
            const FVM_Value &key = roots.values[FVM_MEMBER_TEXT_ROOT];
            if (status == FVM_OK)
            {
                if (c->swf_loaded && owner.type != FVM_VALUE_OBJECT)
                {
                    FVM_Value args[FVM_MEMBER_META_WRITE_ARGUMENTS];
                    args[FVM_MEMBER_META_NAME_ARGUMENT] = key;
                    if (write)
                    {
                        args[FVM_MEMBER_META_ASSIGNED_ARGUMENT] = roots.values[FVM_MEMBER_ASSIGNED_ROOT];
                    }
                    status =
                        fvm_value_meta_invoke(c,
                                              &owner,
                                              write ? "__newindex" : "__index",
                                              write ? FVM_MEMBER_META_WRITE_ARGUMENTS : FVM_MEMBER_META_READ_ARGUMENTS,
                                              args,
                                              &result);
                    if (status == FVM_NOT_FOUND)
                    {
                        FVM_Value &boxed = roots.values[FVM_MEMBER_BOX_ROOT];
                        status = fvm_exec_to_object(c, &owner, &boxed);
                        if (status == FVM_OK && boxed.type != FVM_VALUE_OBJECT)
                        {
                            FVM_Object *bare = 0;
                            status = fvm_object_create(c, &bare);
                            if (status == FVM_OK)
                            {
                                boxed.type = FVM_VALUE_OBJECT;
                                boxed.object = bare;
                            }
                        }
                        if (status == FVM_OK)
                        {
                            status =
                                write
                                    ? fvm_object_set_bytes(c,
                                                           boxed.object,
                                                           key.string,
                                                           fvm_string_size(key),
                                                           &roots.values[FVM_MEMBER_ASSIGNED_ROOT])
                                    : fvm_object_get_bytes(c, boxed.object, key.string, fvm_string_size(key), &result);
                        }
                    }
                    if (status == FVM_OK && write)
                    {
                        fvm_value_undefined(&result);
                    }
                }
                else if (write)
                {
                    status = fvm_value_set_property_bytes(
                        c, &owner, key.string, fvm_string_size(key), &roots.values[FVM_MEMBER_ASSIGNED_ROOT]);
                }
                else
                {
                    status = fvm_value_get_property_bytes(c, &owner, key.string, fvm_string_size(key), &result);
                }
            }
        }
        if (status == FVM_OK)
        {
            *out = result;
        }
        return status;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
