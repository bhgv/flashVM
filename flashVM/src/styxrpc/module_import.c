/* Copyright (c) 2026 bhgv
 * [GC:] EN: Create an ActionScript namespace for a checked remote module interface.
 * RU: Создание пространства ActionScript для проверенного интерфейса удалённого модуля.
 */
#include "module_import.h"
#include "managed_values.h"
#include "flashvm/styxrpc.h"
#include "flashvm/styxrpc_channel.h"
#include "flashvm/userdata.h"
#include "styxrpc/vfs_client.h"
#include <stdlib.h>
#include <string.h>

typedef struct LoadedMethod
{
    union
    {
        FVM_StyxRpcImport integer;    /* Borrowed callback state in a scalar entry. */
        StyxRpcManagedTarget managed; /* Borrowed callback state in all shaped entries. */
    } target;                        /* Selected callable profile. */
    FVM_StyxRpcChannelArguments channels; /* Bridges the managed target to the reverse bus. */
    char name[STYXRPC_TREE_NAME + 1]; /* Owned verified export name. */
} LoadedMethod;

typedef struct LoadedModule
{
    char path[STYXRPC_VFS_PATH_MAX + 1]; /* Owned relative module path. */
    LoadedMethod methods[STYXRPC_MAX_EXPORTS]; /* Stable callback state for native methods. */
} LoadedModule;


/* EN: Match the compiled import and choose the existing AS call adapter.
 * RU: Сверить компилированный импорт и выбрать существующий адаптер вызова AS. */
static FVM_NativeFunction adapter(const FVM_StyxRpcLoadSpec *spec, const StyxRpcModuleImport *method)
{
    switch (method->kind)
    {
    case STYXRPC_MODULE_INT:
        return method->signature == STYXRPC_INT_SIGNATURE ? fvm_styxrpc_invoke_int : NULL;
    case STYXRPC_MODULE_MANAGED:
        return fvm_styxrpc_values_supported(&method->schema) ? fvm_styxrpc_invoke_managed : NULL;
    case STYXRPC_MODULE_GRAPH:
        return fvm_styxrpc_graph_supported(&method->schema) ? fvm_styxrpc_invoke_graph : NULL;
    case STYXRPC_MODULE_CHANNEL:
        return spec->pid && fvm_styxrpc_channel_signature(&method->schema) &&
            (!strchr(method->schema.arguments, 'c') || spec->bus) ? fvm_styxrpc_invoke_channels : NULL;
    }
    return NULL;
}


/* EN: Initialize owned callback storage. Return the exact state for the selected adapter.
 * RU: Инициализировать собственное состояние и вернуть указатель точного адаптера. */
static void *prepare(LoadedModule *module, size_t index, const FVM_StyxRpcLoadSpec *spec)
{
    const StyxRpcModuleImport *entry = spec->methods + index;
    LoadedMethod *method = module->methods + index;
    StyxRpcManagedTarget *target = &method->target.managed;
    strcpy(method->name, entry->name);
    if (entry->kind == STYXRPC_MODULE_INT)
    {
        FVM_StyxRpcImport *scalar = &method->target.integer;
        scalar->session = spec->session;
        scalar->module = module->path;
        scalar->name = method->name;
        scalar->wait_ms = spec->wait_ms;
        return scalar;
    }
    target->session = spec->session;
    target->module = module->path;
    target->name = method->name;
    target->schema = entry->schema;
    target->signature = entry->signature;
    target->pid = spec->pid;
    target->wait_ms = spec->wait_ms;
    if (entry->kind == STYXRPC_MODULE_CHANNEL)
    {
        method->channels.target = target;
        method->channels.bus = spec->bus;
        return &method->channels;
    }
    return target;
}


/* EN: Validate all method names and adapters before checking the mounted interface.
 * RU: Проверить все имена и адаптеры методов перед проверкой смонтированного интерфейса. */
static int checked(const FVM_StyxRpcLoadSpec *spec, FVM_NativeFunction *callbacks)
{
    size_t i, j;
    if (!spec || !spec->session || !spec->path || !*spec->path || strlen(spec->path) > STYXRPC_VFS_PATH_MAX ||
        !spec->methods || !spec->count || spec->count > STYXRPC_MAX_EXPORTS ||
        !spec->wait_ms || spec->wait_ms > STYXRPC_SCALAR_WAIT_MAX_MS)
    {
        return 0;
    }
    for (i = 0; i < spec->count; ++i)
    {
        const StyxRpcModuleImport *method = spec->methods + i;
        if (!method->name || !*method->name || strlen(method->name) > STYXRPC_TREE_NAME ||
            !(callbacks[i] = adapter(spec, method)))
        {
            return 0;
        }
        for (j = 0; j < i; ++j)
        {
            if (!strcmp(method->name, spec->methods[j].name))
            {
                return 0;
            }
        }
    }
    return 1;
}


/* [GC:] EN/RU: See flashvm/styxrpc_module.h. / См. flashvm/styxrpc_module.h. */
FVM_Result fvm_styxrpc_module_load(FVM_Context *context, const FVM_StyxRpcLoadSpec *spec, FVM_Value *out)
{
    FVM_NativeFunction callbacks[STYXRPC_MAX_EXPORTS];
    FVM_GCRootScope *roots;
    FVM_Value value;
    FVM_Result status;
    LoadedModule *module;
    size_t i;
    if (!context || !out || !checked(spec, callbacks))
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (styxrpc_module_check(spec->session, spec->path, spec->methods, spec->count, spec->pid) != STYXRPC_OK)
    {
        return FVM_NOT_FOUND;
    }
    status = fvm_gc_roots_begin(context, &roots);
    if (status != FVM_OK)
    {
        return status;
    }
    fvm_value_undefined(&value);
    status = fvm_object_create(context, &value.object);
    if (status == FVM_OK)
    {
        value.type = FVM_VALUE_OBJECT;
        status = fvm_gc_roots_add(roots, &value);
    }
    if (status == FVM_OK)
    {
        module = calloc(1, sizeof(*module));
        status = module ? fvm_userdata_attach(context, value.object, module, free) : FVM_OUT_OF_MEMORY;
        if (status == FVM_OK)
        {
            strcpy(module->path, spec->path);
            for (i = 0; status == FVM_OK && i < spec->count; ++i)
            {
                void *state = prepare(module, i, spec);
                status = fvm_object_define_method_rooted(context, value.object, module->methods[i].name,
                    callbacks[i], state, value.object);
            }
        }
        else
        {
            free(module);
        }
    }
    if (status == FVM_OK)
    {
        *out = value;
    }
    fvm_gc_roots_end(roots);
    return status;
}


/* [ASINC] EN: Resolve an exact catalog path and use the ordinary typed loader.
 * RU: Разрешить точный путь каталога и вызвать обычный типизированный загрузчик. */
static FVM_Result load_path(FVM_Context *context, FVM_Object *unused, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    const FVM_StyxRpcModuleCatalog *catalog = state;
    const char *path;
    size_t length, index;
    (void)unused;
    if (argc != 1 || args->type != FVM_VALUE_STRING || !args->string)
    {
        return FVM_INVALID_ARGUMENT;
    }
    path = args->string;
    length = fvm_value_string_size(args);
    if (length && path[0] == '/')
    {
        ++path;
        --length;
    }
    for (index = 0; index < catalog->count; ++index)
    {
        const FVM_StyxRpcLoadSpec *spec = catalog->modules + index;
        if (strlen(spec->path) == length && !memcmp(spec->path, path, length))
        {
            return fvm_styxrpc_module_load(context, spec, out);
        }
    }
    return FVM_NOT_FOUND;
}


/* [GC:] EN/RU: See flashvm/styxrpc_module.h. / См. flashvm/styxrpc_module.h. */
FVM_Result fvm_styxrpc_module_loader_install(FVM_Context *context, const char *global,
    const FVM_StyxRpcModuleCatalog *catalog)
{
    FVM_NativeFunction callbacks[STYXRPC_MAX_EXPORTS];
    size_t i, j;
    if (!context || !global || !*global || !catalog || !catalog->modules || !catalog->count)
    {
        return FVM_INVALID_ARGUMENT;
    }
    for (i = 0; i < catalog->count; ++i)
    {
        if (!checked(catalog->modules + i, callbacks) || catalog->modules[i].path[0] == '/')
        {
            return FVM_INVALID_ARGUMENT;
        }
        for (j = 0; j < i; ++j)
        {
            if (!strcmp(catalog->modules[i].path, catalog->modules[j].path))
            {
                return FVM_INVALID_ARGUMENT;
            }
        }
    }
    return fvm_register_function(context, global, load_path, (void *)catalog);
}
