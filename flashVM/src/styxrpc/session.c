/* Copyright (c) 2026 bhgv
 * EN: Own a set of launch fids while publishing one rooted script session.
 * RU: Владение набором fid запуска при публикации объекта сессии в AS.
 */
#include "session_methods.h"
#include "flashvm/gc_roots.h"
#include <string.h>


/* EN/RU: See session_methods.h. / См. session_methods.h. */
int fvm_styxrpc_name(const FVM_Value *value, char *name, size_t capacity)
{
    size_t size;
    if (value->type != FVM_VALUE_STRING)
    {
        return 0;
    }
    size = fvm_value_string_size(value);
    if (!size || size >= capacity || memchr(value->string, 0, size))
    {
        return 0;
    }
    memcpy(name, value->string, size);
    name[size] = 0;
    return 1;
}


/* EN/RU: See session_methods.h. / См. session_methods.h. */
int fvm_styxrpc_number(const FVM_Value *value, uint64_t limit, uint64_t *result)
{
    /* EN: The caller's limit is representable exactly as an AS Number.
     * RU: Предел вызывающего точно представим числом AS. */
    if (value->type != FVM_VALUE_NUMBER || !(value->number >= 0 && value->number <= (double)limit))
    {
        return 0;
    }
    *result = (uint64_t)value->number;
    return (double)*result == value->number;
}


/* [GC:] EN/RU: See flashvm/styxrpc_session.h. / См. flashvm/styxrpc_session.h. */
FVM_Result fvm_styxrpc_session_bind(FVM_Context *context, FVM_StyxRpcSession *binding,
    struct StyxClient *client, const StyxRpcProgram *program, FVM_Value *object)
{
    FVM_GCRootScope *roots;
    FVM_Object *created;
    FVM_Value value;
    FVM_Result status;
    if (!context || !binding || binding->bound || !client || !object ||
        (program && (!program->pid || program->pid == program->parent_pid)))
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = fvm_gc_roots_begin(context, &roots);
    if (status != FVM_OK)
    {
        return status;
    }
    status = fvm_object_create(context, &created);
    if (status == FVM_OK)
    {
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = created;
        status = fvm_gc_roots_add(roots, &value);
    }
    if (status == FVM_OK)
    {
        status = fvm_styxrpc_launch_members(context, created, binding);
    }
    if (status == FVM_OK)
    {
        status = fvm_styxrpc_vfs_members(context, created, binding);
    }
    if (status == FVM_OK)
    {
        binding->client = client;
        if (program)
        {
            binding->program = *program;
        }
        binding->bound = 1;
        *object = value;
    }
    fvm_gc_roots_end(roots);
    return status;
}


/* EN/RU: See flashvm/styxrpc_session.h. / См. flashvm/styxrpc_session.h. */
void fvm_styxrpc_session_close(FVM_StyxRpcSession *binding)
{
    unsigned i;
    if (!binding)
    {
        return;
    }
    binding->client = NULL;
    for (i = 0; i < FVM_STYXRPC_LAUNCH_SLOTS; ++i)
    {
        styxrpc_launch_named_close(&binding->tickets[i].fid);
        binding->tickets[i].token = 0;
    }
}
