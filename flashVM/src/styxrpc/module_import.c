/* Copyright (c) 2026 bhgv
 * [GC:] EN: Create an ActionScript namespace for a checked remote module interface.
 * RU: Создание пространства ActionScript для проверенного интерфейса удалённого модуля.
 */
#include "module_import.h"
#include "managed_values.h"
#include "../capi/userdata_internal.h"
#include "flashvm/styxrpc.h"
#include "flashvm/styxrpc_channel.h"
#include "flashvm/userdata.h"
#include "styxrpc/vfs_client.h"
#include "client.h"
#include "wire_wait.h"
#include <stdlib.h>
#include <string.h>

typedef struct LoadedModule LoadedModule; /* Exact proxy shared by all extracted methods. */

typedef struct LoadedMethod
{
    union
    {
        FVM_StyxRpcImport integer;    /* Borrowed callback state in a scalar entry. */
        StyxRpcManagedTarget managed; /* Borrowed callback state in all shaped entries. */
    } target;                        /* Selected callable profile. */
    FVM_StyxRpcChannelArguments channels; /* Bridges the managed target to the reverse bus. */
    LoadedModule *module;   /* Borrowed rooted proxy throughout the call. */
    StyxRpcServer *reverse; /* Borrowed owner dispatcher for direct scoped calls. */
    StyxRpcModuleKind kind; /* Existing codec chosen from the compiled export. */
    char name[STYXRPC_TREE_NAME + 1]; /* Owned verified export name. */
} LoadedMethod;

struct FVM_StyxModuleLease
{
    struct StyxFid *request; /* Owned control fid through the last module or channel. */
    size_t references;      /* Context-local owner count. */
};

struct LoadedModule
{
    FVM_StyxModuleLease *lease; /* Owned when loaded from an ordinary file. */
    struct StyxFid *directory; /* Owned unopened directory for all method walks. */
    unsigned active;           /* Invocations already holding this directory on the VM stack. */
    int closed;                /* Explicit retirement immediately rejects new invocations. */
    char path[STYXRPC_VFS_PATH_MAX + 1]; /* Owned relative module path. */
    LoadedMethod methods[STYXRPC_MAX_EXPORTS]; /* Stable callback state for native methods. */
};


/* EN/RU: See module_import.h. / См. module_import.h. */
void fvm_styxrpc_live_lease_retain(FVM_StyxModuleLease *lease)
{
    if (lease)
    {
        ++lease->references;
    }
}


/* [GC:] EN/RU: See module_import.h. / См. module_import.h. */
void fvm_styxrpc_live_lease_release(FVM_StyxModuleLease *lease, int deferred)
{
    if (lease && !--lease->references)
    {
        styxrpc_vfs_close(lease->request, deferred);
        free(lease);
    }
}


/* EN: Check whether the AS adapters can represent the borrowed import contract.
 * RU: Проверить представимость заимствованного контракта адаптерами AS. */
static int supported(const FVM_StyxRpcLoadSpec *spec, const StyxRpcModuleImport *method)
{
    switch (method->kind)
    {
    case STYXRPC_MODULE_INT:
        return method->signature == STYXRPC_INT_SIGNATURE;
    case STYXRPC_MODULE_MANAGED:
        return fvm_styxrpc_values_supported(&method->schema);
    case STYXRPC_MODULE_GRAPH:
        return fvm_styxrpc_graph_supported(&method->schema);
    case STYXRPC_MODULE_CHANNEL:
        return spec->pid && fvm_styxrpc_channel_signature(&method->schema) &&
            (!strchr(method->schema.arguments, 'c') || spec->bus);
    }
    return 0;
}


/* EN: Initialize owned callback storage. Return the exact state for the selected adapter.
 * RU: Инициализировать собственное состояние и вернуть указатель точного адаптера. */
static LoadedMethod *prepare(LoadedModule *module, size_t index, const FVM_StyxRpcLoadSpec *spec)
{
    const StyxRpcModuleImport *entry = spec->methods + index;
    LoadedMethod *method = module->methods + index;
    StyxRpcManagedTarget *target = &method->target.managed;
    strcpy(method->name, entry->name);
    method->module = module;
    method->kind = entry->kind;
    if (entry->kind == STYXRPC_MODULE_INT)
    {
        FVM_StyxRpcImport *scalar = &method->target.integer;
        scalar->session = spec->session;
        scalar->module = module->path;
        scalar->name = method->name;
        scalar->wait_ms = spec->wait_ms;
        return method;
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
    }
    return method;
}


/* [GC:] EN: Close owned fids if there are no calls still borrowing them.
 * Use deferred during GC; result channels own independent lease references.
 * RU: Закрыть собственные fid, когда действующие вызовы больше их не заимствуют.
 * При сборке использовать deferred. Каналы результатов отдельно удерживают аренду. */
static void retire(LoadedModule *module, int deferred)
{
    if (!module->active)
    {
        styxrpc_vfs_close(module->directory, deferred);
        fvm_styxrpc_live_lease_release(module->lease, deferred);
        module->directory = NULL;
        module->lease = NULL;
    }
}


/* [GC:] EN: Invoke method from its original module, which stays pinned for the codec.
 * RU: Выполнить метод исходного модуля, удерживаемого на время работы кодека. */
static FVM_Result call(FVM_Context *context, LoadedMethod *method, unsigned argc,
    const FVM_Value *argv, FVM_Value *out)
{
    struct StyxFid *directory = method->module->directory;
    switch (method->kind)
    {
    case STYXRPC_MODULE_INT:
        return fvm_styxrpc_int_at(context, directory, &method->target.integer, argc, argv, out);
    case STYXRPC_MODULE_MANAGED:
        return fvm_styxrpc_managed_at(context, directory, &method->target.managed, argc, argv, out, method->reverse);
    case STYXRPC_MODULE_GRAPH:
        return fvm_styxrpc_graph_at(context, directory, &method->target.managed, argc, argv, out, method->reverse);
    case STYXRPC_MODULE_CHANNEL:
        return fvm_styxrpc_channels_at(context, directory, &method->channels, argc, argv, out,
            method->module->lease, method->reverse);
    }
    return FVM_UNSUPPORTED;
}


/* [GC:] EN: Pin the exact directory across argument conversion and any script reentry.
 * RU: Удержать точный каталог на время преобразования аргументов и повторного входа в скрипт. */
static FVM_Result invoke(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *argv, FVM_Value *out, void *state)
{
    LoadedMethod *method = state;
    LoadedModule *module = method->module;
    FVM_Result result;
    (void)self;
    if (module->closed)
    {
        return fvm_styxrpc_managed_fail(context, "Styx RPC module has been unloaded");
    }
    ++module->active;
    result = call(context, method, argc, argv, out);
    --module->active;
    if (module->closed)
    {
        retire(module, 0);
    }
    return result;
}


/* [GC:] EN: Retire proxy storage, deferring clunk past the collector.
 * RU: Освободить состояние прокси с отложенным clunk после сборщика. */
static void release(void *state)
{
    LoadedModule *module = state;
    retire(module, 1);
    free(module);
}


/* [GC:] EN/RU: See module_import.h. / См. module_import.h. */
FVM_Result fvm_styxrpc_unload_module(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *argv, FVM_Value *out, void *state)
{
    LoadedModule *module;
    (void)self;
    (void)state;
    if (argc != 1 || argv->type != FVM_VALUE_OBJECT)
    {
        return FVM_INVALID_ARGUMENT;
    }
    module = fvm_userdata_get_bound(context, argv->object, release);
    if (!module)
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_boolean(out, !module->closed);
    if (!module->closed)
    {
        module->closed = 1;
        retire(module, 0);
    }
    return FVM_OK;
}


/* EN: Validate all method names and adapters before checking the mounted interface.
 * RU: Проверить все имена и адаптеры методов перед проверкой смонтированного интерфейса. */
static int checked(const FVM_StyxRpcLoadSpec *spec)
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
            !supported(spec, method))
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


/* [GC:] EN/RU: See module_import.h. / См. module_import.h. */
FVM_Result fvm_styxrpc_module_load_at(FVM_Context *context, struct StyxFid *base,
    const FVM_StyxRpcLoadSpec *spec, struct StyxFid **lease, FVM_Value *out, StyxRpcServer *reverse, uint32_t scoped)
{
    FVM_GCRootScope *roots;
    FVM_Value value;
    FVM_Result status;
    LoadedModule *module = NULL;
    struct StyxFid *directory = NULL;
    FVM_Worker *worker = NULL;
    StyxRpcModuleStop stop;
    StyxRpcIoOutcome opened;
    StyxRpcStatus verified;
    size_t i;
    if (!context || !out || !checked(spec))
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_worker_current(context, &worker);
    stop = worker ? fvm_styxrpc_worker_stopped : NULL;
    opened = styxrpc_module_capture_until(spec->session, spec->path, base, &directory, stop, worker);
    if (opened != STYXRPC_IO_OK)
    {
        return opened == STYXRPC_IO_CANCELLED ?
            fvm_styxrpc_managed_fail(context, "Styx RPC module binding cancelled") : FVM_NOT_FOUND;
    }
    verified = styxrpc_module_check_at_until(directory, spec->methods, spec->count, spec->pid, stop, worker);
    if (verified != STYXRPC_OK)
    {
        styxrpc_vfs_close(directory, verified == STYXRPC_CANCELLED);
        return verified == STYXRPC_CANCELLED ?
            fvm_styxrpc_managed_fail(context, "Styx RPC module binding cancelled") : FVM_NOT_FOUND;
    }
    status = fvm_gc_roots_begin(context, &roots);
    if (status != FVM_OK)
    {
        styxrpc_vfs_close(directory, 0);
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
        status = module ? fvm_userdata_attach(context, value.object, module, release) : FVM_OUT_OF_MEMORY;
        if (status == FVM_OK)
        {
            module->directory = directory;
            directory = NULL;
            strcpy(module->path, spec->path);
            for (i = 0; status == FVM_OK && i < spec->count; ++i)
            {
                LoadedMethod *state = prepare(module, i, spec);
                state->reverse = scoped & (UINT32_C(1) << i) ? reverse : NULL;
                status = fvm_object_define_method_rooted(context, value.object, module->methods[i].name,
                    invoke, state, value.object);
            }
        }
        else
        {
            free(module);
            module = NULL;
        }
    }
    if (status == FVM_OK && lease && *lease)
    {
        module->lease = calloc(1, sizeof(*module->lease));
        if (!module->lease)
        {
            status = FVM_OUT_OF_MEMORY;
        }
        else
        {
            module->lease->request = *lease;
            module->lease->references = 1;
            *lease = NULL;
        }
    }
    if (status == FVM_OK)
    {
        *out = value;
    }
    else if (module)
    {
        retire(module, 0);
    }
    fvm_gc_roots_end(roots);
    styxrpc_vfs_close(directory, 0);
    return status;
}


/* [GC:] EN/RU: See flashvm/styxrpc_module.h. / См. flashvm/styxrpc_module.h. */
FVM_Result fvm_styxrpc_module_load(FVM_Context *context, const FVM_StyxRpcLoadSpec *spec, FVM_Value *out)
{
    return fvm_styxrpc_module_load_at(context, NULL, spec, NULL, out, NULL, 0);
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
    size_t i, j;
    if (!context || !global || !*global || !catalog || !catalog->modules || !catalog->count)
    {
        return FVM_INVALID_ARGUMENT;
    }
    for (i = 0; i < catalog->count; ++i)
    {
        if (!checked(catalog->modules + i) || catalog->modules[i].path[0] == '/')
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
