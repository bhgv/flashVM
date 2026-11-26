/* Copyright (c) 2026 bhgv
 * EN: Copy byte requests across the boundary to rooted ActionScript callbacks.
 * RU: Копирование запросов между SDK и удерживаемыми колбэками ActionScript.
 */
#include "session_nodes.h"
#include "../core/runtime.hpp"
#include "flashvm/exception.h"
#include "flashvm/execution.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/gc_roots.h"
#include "styxrpc/nodes.h"
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C"
{
#include "session_methods.h"
}

namespace
{
struct Callback
{
    FVM_Context *context; /* Borrowed movie owner through withdrawal. */
    bool positioned; /* Pass the fourth offset argument to this publication. */
    FVM_Object *function; /* [GC:] Pinned callable, released at withdrawal. */
};


/* [GC:] EN: Unpin an owned callback while its context is still alive.
 * RU: Снять удержание колбэка, пока жив его контекст. */
void release(void *state)
{
    Callback *callback = static_cast<Callback *>(state);
    fvm_gc_unpin(callback->context, callback->function);
    std::free(callback);
}


/* EN: Read each distinct r/w/x letter and produce its Unix permission mask.
 * RU: Прочитать разные буквы r/w/x и собрать маску прав Unix. */
unsigned permission(const FVM_Value *value)
{
    char text[4];
    unsigned mode = 0;
    if (!fvm_styxrpc_name(value, text, sizeof(text)))
    {
        return 0;
    }
    for (const char *letter = text; *letter; ++letter)
    {
        unsigned bit = *letter == 'r' ? 4U : *letter == 'w' ? 2U : *letter == 'x' ? 1U : 0U;
        if (!bit || (mode & bit))
        {
            return 0;
        }
        mode |= bit;
    }
    return mode;
}


/* [GC:] EN: Copy exactly count numeric bytes from buffer, refusing sparse or invalid output.
 * RU: Скопировать ровно count числовых байт из buffer, отклонив пропуски и неверные данные. */
FVM_Result read_reply(FVM_Context *context, FVM_Object *buffer, unsigned char *bytes, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        char key[24];
        int own;
        uint64_t byte;
        FVM_Value value;
        std::sprintf(key, "%lu", static_cast<unsigned long>(i));
        FVM_Result status = fvm_object_has_own(context, buffer, key, &own);
        if (status != FVM_OK || !own)
        {
            return FVM_INVALID_ARGUMENT;
        }
        status = fvm_object_get(context, buffer, key, &value);
        if (status != FVM_OK || !fvm_styxrpc_number(&value, 255, &byte))
        {
            return status == FVM_OK ? FVM_INVALID_ARGUMENT : status;
        }
        bytes[i] = static_cast<unsigned char>(byte);
    }
    return FVM_OK;
}


/* [GC:] EN: Call the movie function with a copied byte array and the opt-in absolute offset.
 * RU: Вызвать функцию фильма со скопированным массивом байтов и смещением, если оно запрошено. */
int transfer_at(void *state, unsigned access, uint64_t offset, void *bytes, size_t size, size_t *used)
{
    Callback *callback = static_cast<Callback *>(state);
    FVM_Context *context = callback->context;
    if (size > UINT_MAX || size > SIZE_MAX / sizeof(FVM_Value))
    {
        return 0;
    }
    FVM_Value *items = size ? static_cast<FVM_Value *>(std::malloc(size * sizeof(*items))) : NULL;
    if (size && !items)
    {
        return 0;
    }
    const unsigned char *source = static_cast<const unsigned char *>(bytes);
    for (size_t i = 0; i < size; ++i)
    {
        fvm_value_number(&items[i], access == STYXRPC_READ ? 0 : source[i]);
    }
    FVM_GCRootScope *roots = NULL;
    FVM_Value args[4], result;
    FVM_Result status = fvm_gc_roots_begin(context, &roots);
    if (status == FVM_OK)
    {
        status = fvm_exec_literal_array(context, static_cast<unsigned>(size), items, args + 1);
    }
    std::free(items);
    if (status == FVM_OK)
    {
        status = fvm_gc_roots_add(roots, args + 1);
    }
    if (status == FVM_OK)
    {
        const char *operation = access == STYXRPC_NODE_EXECUTE ? "x" : access == STYXRPC_WRITE ? "w" : "r";
        fvm_value_string(args, operation);
        fvm_value_number(args + 2, static_cast<double>(size));
        fvm_value_number(args + 3, static_cast<double>(offset));
        status = fvm_function_invoke(context, callback->function, NULL, callback->positioned ? 4 : 3, args, &result);
    }
    uint64_t count = 0;
    if (status == FVM_OK && !fvm_styxrpc_number(&result, size, &count))
    {
        status = FVM_INVALID_ARGUMENT;
    }
    if (status == FVM_OK && access != STYXRPC_WRITE)
    {
        status = read_reply(context, args[1].object, static_cast<unsigned char *>(bytes), static_cast<size_t>(count));
    }
    if (roots)
    {
        fvm_gc_roots_end(roots);
    }
    if (status == FVM_EXCEPTION)
    {
        fvm_exception_clear(context);
    }
    if (status == FVM_OK)
    {
        *used = static_cast<size_t>(count);
    }
    return status == FVM_OK;
}
/* EN: Keep the original three-argument callable for a sequential registration.
 * RU: Сохранить исходный вызов с тремя аргументами для последовательной регистрации. */
int transfer(void *state, unsigned access, void *bytes, size_t size, size_t *used)
{
    return transfer_at(state, access, 0, bytes, size, used);
}
} // namespace


/* [GC:] EN/RU: See session_nodes.h. / См. session_nodes.h. */
extern "C" FVM_Result fvm_styxrpc_publish_node(FVM_Context *context, StyxRpcModuleSlot *slot,
    unsigned argc, const FVM_Value *args, FVM_Value *out)
{
    char path[STYXRPC_NODE_PATH_MAX + 2];
    uint64_t length = 0;
    if ((argc != 3 && argc != 4) ||
        (argc == 4 && !fvm_styxrpc_number(args + 3, STYXRPC_NODE_OFFSET_MAX, &length)) ||
        !fvm_styxrpc_name(args, path, sizeof(path)) || args[2].type != FVM_VALUE_OBJECT ||
        !args[2].object || !args[2].object->invocable.callback)
    {
        return FVM_INVALID_ARGUMENT;
    }
    unsigned mode = permission(args + 1);
    if (!mode)
    {
        return FVM_INVALID_ARGUMENT;
    }
    Callback *callback = static_cast<Callback *>(std::malloc(sizeof(*callback)));
    if (!callback)
    {
        return FVM_OUT_OF_MEMORY;
    }
    callback->context = context;
    callback->positioned = argc == 4;
    callback->function = args[2].object;
    FVM_Result status = fvm_gc_pin(context, callback->function);
    if (status != FVM_OK)
    {
        std::free(callback);
        return status;
    }
    StyxRpcStatus published;
    if (callback->positioned)
    {
        const StyxRpcPositionedNodeCallbacks config = {transfer_at, release, callback};
        published = styxrpc_module_slot_new_node_at(slot, path, mode, length, &config);
    }
    else
    {
        const StyxRpcNodeCallbacks config = {transfer, release, callback};
        published = styxrpc_module_slot_new_node(slot, path, mode, &config);
    }
    if (published != STYXRPC_OK)
    {
        release(callback);
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_boolean(out, 1);
    return FVM_OK;
}
