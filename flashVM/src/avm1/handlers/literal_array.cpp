/* FlashVM: value-only Array literal construction for interpreted and generated callers. */
#include "support.hpp"
#include "literal_constants.h"
#include "../../standard/base_state.hpp"


/* Define own indexed slots without invoking inherited setters or numeric-length overloads. */
extern "C" FVM_Result
fvm_exec_literal_array(FVM_Context *c, unsigned int count, const FVM_Value *values, FVM_Value *out)
{
    if (!c || c->destroying || c->gc_releasing || !out || count > FVM_EXEC_MAX_LITERAL_ENTRIES || (count && !values))
    {
        return FVM_INVALID_ARGUMENT;
    }
    for (unsigned int i = 0; i < count; ++i)
    {
        if (fvm_validate_value(c, values + i) != FVM_OK)
        {
            return FVM_INVALID_ARGUMENT;
        }
    }
    try
    {


        /* [GC:] Snapshot every input before any constructor/prototype work can run. */
        FVM_ExecutionRoots roots(c, (size_t)count + FVM_LITERAL_ARRAY_VALUE_BASE);
        for (unsigned int i = 0; i < count; ++i)
        {
            roots.values[FVM_LITERAL_ARRAY_VALUE_BASE + i] = values[i];
        }
        FVM_Object *array = 0;
        FVM_Result status = fvm_array_create_like(c, 0, count, &array, true);
        if (status != FVM_OK)
        {
            return status;
        }
        FVM_Value &result = roots.values[FVM_LITERAL_RESULT_ROOT];
        result.type = FVM_VALUE_OBJECT;
        result.object = array;
        for (unsigned int i = 0; i < count; ++i)
        {
            status = fvm_array_store_own(c, array, i, roots.values[FVM_LITERAL_ARRAY_VALUE_BASE + i]);
            if (status != FVM_OK)
            {
                return status;
            }
        }
        *out = result;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
