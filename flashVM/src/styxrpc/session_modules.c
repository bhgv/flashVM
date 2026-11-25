/* Copyright (c) 2026 bhgv
 * EN: Checked module imports and live Int32 variables on a movie's existing session.
 * RU: Проверяемый импорт модулей и живые переменные Int32 в действующей сессии фильма.
 */
#include "session_methods.h"
#include "variable_ref_adapter.h"
#include "flashvm/styxrpc_filesystem.h"
#include "flashvm/styxrpc_module.h"
#include "styxrpc/vfs_client.h"
#include <string.h>


/* EN: Copy one method contract and its name from a dense AS descriptor.
 * RU: Скопировать контракт одного метода и его имя из плотного описателя AS. */
static FVM_Result import_descriptor(FVM_Context *context, const FVM_Value *descriptor,
    StyxRpcModuleImport *method, char *name)
{
    FVM_Value fields[5];
    FVM_GCRootScope *roots;
    uint64_t count, signature;
    char profile[8];
    FVM_Result status = fvm_gc_roots_begin(context, &roots);
    if (status != FVM_OK)
    {
        return status;
    }
    status = fvm_styxrpc_fields(context, roots, descriptor, fields, 5, &count);
    if (status == FVM_OK)
    {
        if ((count != 1 && count != 5) || !fvm_styxrpc_name(fields, name, STYXRPC_TREE_NAME + 1) ||
            strchr(name, '/') || name[0] == '.')
        {
            status = FVM_INVALID_ARGUMENT;
        }
        else if (count == 1)
        {
            method->kind = STYXRPC_MODULE_INT;
            method->signature = STYXRPC_INT_SIGNATURE;
        }
        else if (!fvm_styxrpc_name(fields + 1, profile, sizeof(profile)) ||
            (strcmp(profile, "managed") && strcmp(profile, "graph")) ||
            !fvm_styxrpc_number(fields + 2, UINT32_MAX, &signature) || !signature ||
            !fvm_styxrpc_name(fields + 3, method->schema.arguments, sizeof(method->schema.arguments)) ||
            !fvm_styxrpc_name(fields + 4, method->schema.result, sizeof(method->schema.result)))
        {
            status = FVM_INVALID_ARGUMENT;
        }
        else
        {
            method->kind = !strcmp(profile, "graph") ? STYXRPC_MODULE_GRAPH : STYXRPC_MODULE_MANAGED;
            method->signature = (uint32_t)signature;
        }
    }
    fvm_gc_roots_end(roots);
    method->name = name;
    return status;
}


/* [ASINC] [GC:] EN: Load a checked namespace by path and exact method descriptors on the owner.
 * RU: Загрузить проверяемое пространство по пути и точным описателям методов у владельца. */
static FVM_Result load_module(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    FVM_StyxRpcSession *binding = state;
    StyxRpcModuleImport methods[STYXRPC_MAX_EXPORTS] = {0};
    char names[STYXRPC_MAX_EXPORTS][STYXRPC_TREE_NAME + 1];
    char path[STYXRPC_VFS_PATH_MAX + 2];
    FVM_StyxRpcLoadSpec spec = {0};
    unsigned i;
    (void)self;
    if (!binding->client || argc < 2 || argc - 1 > STYXRPC_MAX_EXPORTS ||
        !fvm_styxrpc_name(args, path, sizeof(path)))
    {
        return FVM_INVALID_ARGUMENT;
    }
    for (i = 0; i < argc - 1; ++i)
    {
        FVM_Result status = import_descriptor(context, args + i + 1, methods + i, names[i]);
        if (status != FVM_OK)
        {
            return status;
        }
    }
    spec.session = binding->client;
    spec.path = path + (path[0] == '/');
    spec.methods = methods;
    spec.count = argc - 1;
    spec.pid = binding->program.pid;
    spec.wait_ms = FVM_SESSION_CALL_WAIT_MS;
    return fvm_styxrpc_module_load(context, &spec, out);
}


/* [ASINC] [GC:] EN: Open the exact .var path and return a live value accessor, read-only by default.
 * RU: Открыть точный путь .var и вернуть живое свойство value, по умолчанию только для чтения. */
static FVM_Result int_variable(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    FVM_StyxRpcSession *binding = state;
    char path[STYXRPC_VFS_PATH_MAX + 2];
    FVM_GCRootScope *roots;
    FVM_Value proxy;
    FVM_Result status;
    (void)self;
    if (!binding->client || argc < 1 || argc > 2 || (argc == 2 && args[1].type != FVM_VALUE_BOOLEAN) ||
        !fvm_styxrpc_name(args, path, sizeof(path)))
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = fvm_gc_roots_begin(context, &roots);
    if (status != FVM_OK)
    {
        return status;
    }
    fvm_value_undefined(&proxy);
    status = fvm_object_create(context, &proxy.object);
    if (status == FVM_OK)
    {
        proxy.type = FVM_VALUE_OBJECT;
        status = fvm_gc_roots_add(roots, &proxy);
    }
    if (status == FVM_OK)
    {
        status = fvm_styxrpc_int_variable_bind(context, proxy.object, "value", binding->client,
            path + (path[0] == '/'), argc == 2 && args[1].boolean);
    }
    if (status == FVM_OK)
    {
        *out = proxy;
    }
    fvm_gc_roots_end(roots);
    return status;
}


/* EN/RU: See session_methods.h. / См. session_methods.h. */
FVM_Result fvm_styxrpc_module_members(FVM_Context *context, FVM_Object *object, FVM_StyxRpcSession *binding)
{
    FVM_Result status = fvm_object_define_method_rooted(context, object, "loadModule", load_module, binding, NULL);
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(context, object, "intVariable", int_variable, binding, NULL);
    }
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(context, object, "acceptIntRef", fvm_styxrpc_accept_int_ref, binding, NULL);
    }
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(context, object, "intRefBytes", fvm_styxrpc_int_ref_bytes, binding, NULL);
    }
    return status;
}
