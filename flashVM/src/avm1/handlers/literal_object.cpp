/* FlashVM: value-only Object literal construction with ordered name conversion and setters. */
#include "support.hpp"
#include "literal_constants.h"
#include "flashvm/standard.h"
/* Retain a complete entry snapshot through constructor, coercion and setter callbacks. */
extern "C" FVM_Result fvm_exec_literal_object(FVM_Context *c, unsigned int count,
                                              const FVM_ExecutionLiteralEntry *entries,
                                              FVM_Value *out)
{
    if (!c || c->destroying || c->gc_releasing || !out || count > FVM_EXEC_MAX_LITERAL_ENTRIES ||
        (count && !entries))
        return FVM_INVALID_ARGUMENT;
    for (unsigned int i = 0; i < count; ++i)
        if (fvm_validate_value(c, &entries[i].name) != FVM_OK ||
            fvm_validate_value(c, &entries[i].value) != FVM_OK)
            return FVM_INVALID_ARGUMENT;
    try
    {
        /* [GC:] Root all entries before Object construction or any property-name callback. */
        FVM_ExecutionRoots roots(c, (size_t)count * FVM_LITERAL_OBJECT_ENTRY_WIDTH +
                                        FVM_LITERAL_OBJECT_ENTRY_BASE);
        for (unsigned int i = 0; i < count; ++i)
        {
            size_t slot =
                FVM_LITERAL_OBJECT_ENTRY_BASE + (size_t)i * FVM_LITERAL_OBJECT_ENTRY_WIDTH;
            roots.values[slot] = entries[i].name;
            roots.values[slot + FVM_LITERAL_OBJECT_VALUE_OFFSET] = entries[i].value;
        }
        FVM_Object *object = 0;
        FVM_Result status = fvm_construct(c, "Object", 0, 0, &object);
        if (status != FVM_OK)
            return status;
        FVM_Value &result = roots.values[FVM_LITERAL_RESULT_ROOT];
        result.type = FVM_VALUE_OBJECT;
        result.object = object;
        FVM_Value &key = roots.values[FVM_LITERAL_OBJECT_KEY_ROOT];
        for (unsigned int i = 0; i < count; ++i)
        {
            size_t slot =
                FVM_LITERAL_OBJECT_ENTRY_BASE + (size_t)i * FVM_LITERAL_OBJECT_ENTRY_WIDTH;
            fvm_value_undefined(&key);
            status = c->swf_loaded ? fvm_avm1_string(c, roots.values[slot], &key)
                                   : fvm_value_to_string(c, &roots.values[slot], &key);
            if (status != FVM_OK)
                return status;
            if (c->swf_loaded && !fvm_string_size(key))
                continue;
            status = fvm_object_set_bytes(c, object, key.string, fvm_string_size(key),
                                          &roots.values[slot + FVM_LITERAL_OBJECT_VALUE_OFFSET]);
            if (status != FVM_OK)
                return status;
        }
        *out = result;
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}
