/* Copyright (c) 2026 bhgv
 * EN: Launch detached AVM1 entries using counted graph snapshots.
 * RU: Запуск отделённых функций AVM1 из снимков графов.
 */
#include <stdlib.h>
#include "../standard/async_worker.h"
#include "flashvm/execution.h"
#include "flashvm/gc.h"
#include "flashvm/table.h"
#include "flashvm/serialization.h"
#include "flashvm/standard.h"
#include "../core/capability_internal.hpp"

#define FVM_WORKER_NIBBLE_BITS 4 /* Bits represented by one transport letter. */
#define FVM_WORKER_NIBBLE_MASK 15 /* Low-nibble transport mask. */
#define FVM_WORKER_BYTE_LETTERS 2 /* UTF-8 letters per serialized byte. */


/* [GC:] EN: Restore ordered positional inputs from the rooted graph array.
 * RU: Восстановить позиционные аргументы из удерживаемого массива графа. */
static FVM_Result invoke_arguments(FVM_Context *context, FVM_Object *packet, FVM_Value *output)
{
    FVM_Value length;
    FVM_Result status = fvm_object_get(context, packet, "length", &length);
    if (status != FVM_OK)
    {
        return status;
    }
    if (length.type != FVM_VALUE_NUMBER || !(length.number >= 2 &&
        length.number <= 1 + FVM_WORKER_FUNCTION_INPUTS))
    {
        return FVM_INVALID_ARGUMENT;
    }
    unsigned int count = static_cast<unsigned int>(length.number);
    if (count != length.number)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Value values[1 + FVM_WORKER_FUNCTION_INPUTS];
    for (unsigned int i = 0; i < count; ++i)
    {
        status = fvm_table_get_index(context, packet, i, values + i);
        if (status != FVM_OK)
        {
            return status;
        }
    }
    if (values[0].type != FVM_VALUE_OBJECT || !values[0].object ||
        values[0].object->resource_kind != FVM_RESOURCE_SCRIPT_FUNCTION)
    {
        return FVM_UNSUPPORTED;
    }
    return fvm_function_invoke(context, values[0].object, 0, count - 1, values + 1, output);
}


/* Restore the copied closure or argument packet in the worker context.
 * RU: Восстановить замыкание либо пакет аргументов в контексте потока.
 * context/input/output are worker-owned; state borrows immutable native setup. Return the
 * first restoration/execution failure. The worker layer copies primitive output
 * before destroying its context and rejects object results. No owner dispatch. */
static FVM_Result invoke_snapshot(FVM_Context *context, const FVM_Value *input,
                                  FVM_Value *output, void *state)
{
    FVM_Value function;
    FVM_Result result;
    size_t size = fvm_value_string_size(input) / FVM_WORKER_BYTE_LETTERS;
    unsigned char *bytes;
    size_t i;
    const FVM_AsyncWorkerSetup *setup = static_cast<const FVM_AsyncWorkerSetup *>(state);
    result = fvm_standard_install(context);
    if (result == FVM_OK && setup && setup->install)
    {
        result = setup->install(context, setup->state);
    }
    if (result != FVM_OK)
    {
        return result;
    }
    bytes = (unsigned char *)malloc(size);
    if (!bytes)
    {
        return FVM_OUT_OF_MEMORY;
    }
    /* Launch encodes each byte as two letters a..p, a counted UTF-8 string. */
    for (i = 0; i < size; ++i)
    {
        unsigned int high = (unsigned int)(input->string[FVM_WORKER_BYTE_LETTERS * i] - 'a');
        unsigned int low = (unsigned int)(input->string[FVM_WORKER_BYTE_LETTERS * i + 1] - 'a');
        bytes[i] = (unsigned char)((high << FVM_WORKER_NIBBLE_BITS) | low);
    }
    result = fvm_value_deserialize(context, bytes, size, &function);
    free(bytes);
    if (result != FVM_OK)
    {
        return result;
    }
    if (function.type != FVM_VALUE_OBJECT || !function.object)
    {
        return FVM_UNSUPPORTED;
    }
    if (function.object->resource_kind == FVM_RESOURCE_SCRIPT_FUNCTION)
    {
        return fvm_function_invoke(context, function.object, 0, 0, 0, output);
    }
    result = fvm_gc_pin(context, function.object);
    if (result == FVM_OK)
    {
        result = invoke_arguments(context, function.object, output);
        fvm_gc_unpin(context, function.object);
    }
    return result;
}


/* [GC:] EN/RU: See async_worker.h. / См. async_worker.h. */
extern "C" FVM_Result fvm_worker_run_prepared(FVM_Context *owner, const FVM_Config *config,
    FVM_Object *function, unsigned int count, const FVM_Value *args, FVM_AsyncWorkerSetup *setup,
    FVM_Worker **out)
{
    unsigned char *bytes = 0;
    size_t size = 0;
    size_t i;
    char *encoded = 0;
    FVM_Value value;
    FVM_Value input;
    FVM_Result result;
    if (!out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *out = 0;
    if (!owner || !config || config->struct_size != sizeof(FVM_Config) ||
        !function || function->owner != owner || count > FVM_WORKER_FUNCTION_INPUTS || (count && !args))
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (function->resource_kind != FVM_RESOURCE_SCRIPT_FUNCTION ||
        function->script_body.callback || fvm_function_transfer_enabled(owner))
    {
        return FVM_UNSUPPORTED;
    }
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = function;
    if (count)
    {
        FVM_Value values[1 + FVM_WORKER_FUNCTION_INPUTS];
        values[0] = value;
        for (unsigned int n = 0; n < count; ++n)
        {
            values[n + 1] = args[n];
        }
        result = fvm_exec_literal_array(owner, count + 1, values, &value);
        if (result != FVM_OK)
        {
            return result;
        }
        result = fvm_gc_pin(owner, value.object);
        if (result != FVM_OK)
        {
            return result;
        }
        result = fvm_value_serialize(owner, &value, &bytes, &size);
        fvm_gc_unpin(owner, value.object);
    }
    else
    {
        result = fvm_value_serialize(owner, &value, &bytes, &size);
    }
    if (result == FVM_OK)
    {
        if (size > ((size_t)-1 - 1) / FVM_WORKER_BYTE_LETTERS)
        {
            fvm_serialized_free(bytes);
            return FVM_OUT_OF_MEMORY;
        }
        encoded = (char *)malloc(FVM_WORKER_BYTE_LETTERS * size + 1);
        if (!encoded)
        {
            fvm_serialized_free(bytes);
            return FVM_OUT_OF_MEMORY;
        }
        for (i = 0; i < size; ++i)
        {
            encoded[FVM_WORKER_BYTE_LETTERS * i] = (char)('a' + (bytes[i] >> FVM_WORKER_NIBBLE_BITS));
            encoded[FVM_WORKER_BYTE_LETTERS * i + 1] = (char)('a' + (bytes[i] & FVM_WORKER_NIBBLE_MASK));
        }
        encoded[FVM_WORKER_BYTE_LETTERS * size] = 0;
        result = fvm_value_string_bytes(&input, encoded, FVM_WORKER_BYTE_LETTERS * size);
        if (result == FVM_OK)
        {
            result = fvm_worker_run(config, invoke_snapshot, &input, setup, out);
        }
    }
    free(encoded);
    fvm_serialized_free(bytes);
    return result;
}

/* [GC:] EN/RU: See worker.h. / См. worker.h. */
extern "C" FVM_Result fvm_worker_run_function_args(FVM_Context *owner, const FVM_Config *config,
    FVM_Object *function, unsigned int count, const FVM_Value *args, FVM_Worker **out)
{
    return fvm_worker_run_prepared(owner, config, function, count, args, 0, out);
}


/* [GC:] EN/RU: See worker.h. / См. worker.h. */
extern "C" FVM_Result fvm_worker_run_function(FVM_Context *owner, const FVM_Config *config,
    FVM_Object *function, FVM_Worker **out)
{
    return fvm_worker_run_function_args(owner, config, function, 0, 0, out);
}
