/* FlashVM: call frame semantics for C callers and generated code. */
#include "support.hpp"
#include "execution_roots.h"
/* Execute value semantics with validation and allocation failure containment. */
extern "C" FVM_Result fvm_exec_call_frame(FVM_Context *c, FVM_Object *scope_receiver,
                                          const FVM_Value *input)
{
    if (!c || (scope_receiver && scope_receiver->owner != c) ||
        fvm_validate_value(c, input) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    try
    {
        FVM_ExecutionRoots roots(c, FVM_CALL_FRAME_ROOT_COUNT);
        roots.values[FVM_CALL_FRAME_ARGUMENT_ROOT] = *input;
        if (scope_receiver)
        {
            roots.values[FVM_CALL_FRAME_RECEIVER_ROOT].type = FVM_VALUE_OBJECT;
            roots.values[FVM_CALL_FRAME_RECEIVER_ROOT].object = scope_receiver;
        }
        FVM_Value argument = roots.values[FVM_CALL_FRAME_ARGUMENT_ROOT];
        FVM_Result status = FVM_OK;
        if (argument.type != FVM_VALUE_NUMBER)
        {
            FVM_Value converted;
            status = fvm_avm1_string(c, argument, &converted);
            if (status == FVM_OK)
                argument = converted;
        }
        if (status != FVM_OK)
            return status;
        /* [GC:] Keep the converted label live until the nested frame has finished. */
        roots.values[FVM_CALL_FRAME_ARGUMENT_ROOT] = argument;
        FVM_Object *target =
            scope_receiver && scope_receiver->resource_kind == FVM_RESOURCE_MOVIECLIP
                ? scope_receiver
                : 0;
        if (!target)
        {
            FVM_Value root;
            status = fvm_get_global(c, "_root", &root);
            if (status != FVM_OK)
                return status;
            if (root.type == FVM_VALUE_OBJECT &&
                root.object->resource_kind == FVM_RESOURCE_MOVIECLIP)
                target = root.object;
        }
        if (!target)
            return FVM_UNSUPPORTED;
        if (argument.type == FVM_VALUE_STRING)
        {
            size_t size = fvm_string_size(argument), separator = size;
            for (size_t part = 0; part < size; ++part)
                if (argument.string[part] == ':')
                    separator = part;
            if (separator < size)
            {
                FVM_Value path, frame;
                status = fvm_value_string_bytes(&path, argument.string, separator);
                if (status == FVM_OK)
                    status = fvm_value_string_bytes(&frame, argument.string + separator + 1,
                                                    size - separator - 1);
                FVM_Object *resolved = 0;
                if (status == FVM_OK)
                    status = fvm_movieclip_resolve_path(c, target, path, &resolved);
                if (status != FVM_OK)
                    return status;
                target =
                    resolved && resolved->resource_kind == FVM_RESOURCE_MOVIECLIP ? resolved : 0;
                argument = frame;
            }
        }
        status = target ? fvm_movieclip_call_frame(c, target, argument) : FVM_OK;
        if (status != FVM_OK)
            return status;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
