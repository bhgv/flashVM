/* Detached AVM1 execution: graph bytes, never owner-context pointers, cross threads. */
#include <stdlib.h>
#include "flashvm/worker.h"
#include "flashvm/serialization.h"
#include "flashvm/standard.h"
#include "../core/capability_internal.hpp"

#define FVM_WORKER_NIBBLE_BITS 4 /* Bits represented by one transport letter. */
#define FVM_WORKER_NIBBLE_MASK 15 /* Low-nibble transport mask. */
#define FVM_WORKER_BYTE_LETTERS 2 /* UTF-8 letters per serialized byte. */


/* Restore the copied closure in the worker context and invoke without arguments.
 * context/input/output are worker-owned; unused carries no host state. Return the
 * first restoration/execution failure. The worker layer copies primitive output
 * before destroying its context and rejects object results. No owner dispatch. */
static FVM_Result invoke_snapshot(FVM_Context *context, const FVM_Value *input,
                                  FVM_Value *output, void *unused)
{
    FVM_Value function;
    FVM_Result result;
    size_t size = fvm_value_string_size(input) / FVM_WORKER_BYTE_LETTERS;
    unsigned char *bytes;
    size_t i;
    (void)unused;
    result = fvm_standard_install(context);
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
    if (function.type != FVM_VALUE_OBJECT || !function.object ||
        function.object->resource_kind != FVM_RESOURCE_SCRIPT_FUNCTION)
    {
        return FVM_UNSUPPORTED;
    }
    return fvm_function_invoke(context, function.object, 0, 0, 0, output);
}


/* Snapshot a same-context AVM1 entry before starting its independent OS worker.
 * owner/config/function belong to the caller; out receives an owned handle only
 * on success. No owner pointer or native attachment crosses the boundary. The
 * synchronous serialization may fail before launch; serialized storage is freed
 * after worker_run has copied it. See worker.h for lifecycle and scope limits. */
extern "C" FVM_Result fvm_worker_run_function(FVM_Context *owner,
                                               const FVM_Config *config,
                                               FVM_Object *function,
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
        !function || function->owner != owner)
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
    result = fvm_value_serialize(owner, &value, &bytes, &size);
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
            result = fvm_worker_run(config, invoke_snapshot, &input, 0, out);
        }
    }
    free(encoded);
    fvm_serialized_free(bytes);
    return result;
}
