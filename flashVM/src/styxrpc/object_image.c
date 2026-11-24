/* Copyright (c) 2026 bhgv
 * [GC:] EN: Retain one FlashVM receiver for multiple ordinary module callbacks.
 * RU: Удержание одного объекта FlashVM для нескольких обычных колбэков модуля.
 */
#include "flashvm/styxrpc_module.h"
#include "flashvm/exception.h"
#include "flashvm/gc.h"
#include "object_handlers.h"
#include <stdlib.h>
#include <string.h>

typedef struct ImageMethod
{
    FVM_Object *object;                 /* Borrowed from the image pin. */
    char name[STYXRPC_TREE_NAME + 1];   /* Owned ActionScript member name. */
    union
    {
        FVM_StyxRpcExport integer;         /* Scalar export binding. */
        FVM_StyxRpcManagedExport managed;  /* Managed or graph export binding. */
        FVM_StyxRpcChannelExport channel; /* Live result initializer binding. */
    } binding;                          /* Selected export profile. */
} ImageMethod;

typedef struct ImageVariable
{
    FVM_Context *context;               /* Borrowed owner. */
    FVM_Object *object;                 /* Borrowed from the image pin. */
    char name[STYXRPC_TREE_NAME + 1];   /* Owned ActionScript property name. */
} ImageVariable;

struct FVM_StyxRpcObjectImage
{
    FVM_Context *context;        /* Borrowed before teardown. */
    FVM_Object *object;          /* Pinned until the image is destroyed. */
    StyxRpcModule *module;        /* Owned protocol image. */
    ImageMethod methods[STYXRPC_MAX_EXPORTS];       /* Stable native callback state. */
    ImageVariable variables[STYXRPC_MAX_EXPORTS];  /* Stable variable callback state. */
    unsigned char reply[STYXRPC_MANAGED_LIMIT];    /* Owner-only output staging until the codec copies it. */
};


/* [ASINC] EN: Dispatch an int entry on its retained receiver.
 * RU: Выполнить скалярный вход на удерживаемом объекте. */
static int invoke_int(void *state, int64_t argument, int64_t *result)
{
    ImageMethod *method = state;
    return fvm_styxrpc_run_int(&method->binding.integer, method->object, argument, result);
}


/* [ASINC] EN: Dispatch a managed entry on its retained receiver.
 * RU: Выполнить управляемый вход на удерживаемом объекте. */
static StyxRpcStatus invoke_managed(void *state, const FVM_SRecord *arguments, StyxRpcOutput *result)
{
    ImageMethod *method = state;
    return fvm_styxrpc_run_managed(&method->binding.managed, method->object, arguments, result);
}


/* [ASINC] EN: Dispatch a graph entry on its retained receiver.
 * RU: Выполнить вход графа на удерживаемом объекте. */
static StyxRpcStatus invoke_graph(void *state, const FVM_SRecord *arguments, StyxRpcGraphOutput *result)
{
    ImageMethod *method = state;
    return fvm_styxrpc_run_graph(&method->binding.managed, method->object, arguments, result);
}


/* [ASINC] EN: Dispatch a channel factory on its retained receiver.
 * RU: Выполнить фабрику канала на удерживаемом объекте. */
static StyxRpcStatus invoke_channel(void *state, const StyxRpcChannelCall *call, StyxRpcChannelOutput *result)
{
    ImageMethod *method = state;
    return fvm_styxrpc_run_channel(&method->binding.channel, method->object, call, result);
}


/* [ASINC] EN: Load the exact int property; isolate exceptions raised by script accessors.
 * RU: Прочитать точное свойство int, изолируя исключения геттера. */
static int read_variable(void *state, int32_t *out)
{
    const ImageVariable *variable = state;
    FVM_Value value;
    FVM_Result status;
    int32_t number;
    if (fvm_exception_get(variable->context, &value) != FVM_NOT_FOUND)
    {
        return 0;
    }
    status = fvm_object_get(variable->context, variable->object, variable->name, &value);
    if (status == FVM_EXCEPTION)
    {
        fvm_exception_clear(variable->context);
    }
    if (status != FVM_OK || value.type != FVM_VALUE_NUMBER ||
        !(value.number >= INT32_MIN && value.number <= INT32_MAX))
    {
        return 0;
    }
    number = (int32_t)value.number;
    if ((double)number != value.number)
    {
        return 0;
    }
    *out = number;
    return 1;
}


/* [ASINC] EN: Write one exact property on the owner and consume any operation exception.
 * RU: Записать одно точное свойство у владельца и поглотить исключение этой операции. */
static int write_variable(void *state, int32_t number)
{
    const ImageVariable *variable = state;
    FVM_Value value;
    FVM_Result status;
    if (fvm_exception_get(variable->context, &value) != FVM_NOT_FOUND)
    {
        return 0;
    }
    fvm_value_number(&value, (double)number);
    status = fvm_object_set(variable->context, variable->object, variable->name, &value);
    if (status == FVM_EXCEPTION)
    {
        fvm_exception_clear(variable->context);
    }
    return status == FVM_OK;
}


/* EN: Prepare one method's immutable descriptor using the existing export codecs.
 * RU: Подготовить неизменяемый описатель метода штатными кодеками экспорта. */
static FVM_Result bind_method(FVM_StyxRpcObjectImage *image, size_t index,
    const FVM_StyxRpcObjectMethod *source, StyxRpcModuleEntry *entry)
{
    ImageMethod *method = image->methods + index;
    FVM_Result status;
    if (!source->method || !*source->method || strlen(source->method) >= sizeof(method->name))
    {
        return FVM_INVALID_ARGUMENT;
    }
    strcpy(method->name, source->method);
    method->object = image->object;
    entry->kind = source->kind;
    switch (source->kind)
    {
    case STYXRPC_MODULE_INT:
        if (source->signature != STYXRPC_INT_SIGNATURE)
        {
            return FVM_INVALID_ARGUMENT;
        }
        method->binding.integer.context = image->context;
        method->binding.integer.function = method->name;
        status = fvm_styxrpc_export_int(&method->binding.integer, source->name, &entry->callback.integer);
        if (status == FVM_OK)
        {
            entry->callback.integer.invoke = invoke_int;
            entry->callback.integer.context = method;
        }
        return status;
    case STYXRPC_MODULE_CHANNEL:
        method->binding.channel.context = image->context;
        method->binding.channel.initializer = method->name;
        method->binding.channel.schema = source->schema;
        method->binding.channel.capacity = source->slots;
        status = fvm_styxrpc_channel_export(&method->binding.channel, source->name, source->signature,
            &entry->callback.channel);
        if (status == FVM_OK)
        {
            entry->callback.channel.begin = invoke_channel;
            entry->callback.channel.context = method;
        }
        return status;
    case STYXRPC_MODULE_MANAGED:
    case STYXRPC_MODULE_GRAPH:
        break;
    default:
        return FVM_INVALID_ARGUMENT;
    }
    method->binding.managed.context = image->context;
    method->binding.managed.function = method->name;
    method->binding.managed.schema = source->schema;
    method->binding.managed.result_buffer = image->reply;
    method->binding.managed.result_capacity = sizeof(image->reply);
    if (source->kind == STYXRPC_MODULE_MANAGED)
    {
        status = fvm_styxrpc_export_managed(&method->binding.managed, source->name, source->signature,
            &entry->callback.managed);
        if (status == FVM_OK)
        {
            entry->callback.managed.begin = invoke_managed;
            entry->callback.managed.context = method;
        }
    }
    else
    {
        status = fvm_styxrpc_export_graph(&method->binding.managed, source->name, source->signature, &entry->callback.graph);
        if (status == FVM_OK)
        {
            entry->callback.graph.begin = invoke_graph;
            entry->callback.graph.context = method;
        }
    }
    return status;
}


/* EN: Prepare the supplied variable descriptors. The provider checks distinct public names.
 * RU: Подготовить описатели переменных. Провайдер проверяет уникальность публичных имён. */
static int bind_variables(FVM_StyxRpcObjectImage *image, const FVM_StyxRpcObjectSpec *spec,
    StyxRpcModuleVariable *variables)
{
    size_t i;
    for (i = 0; i < spec->variable_count; ++i)
    {
        const FVM_StyxRpcObjectVariable *source = spec->variables + i;
        ImageVariable *variable = image->variables + i;
        if (!source->property || !*source->property || strlen(source->property) >= sizeof(variable->name) ||
            (source->writable != 0 && source->writable != 1))
        {
            return 0;
        }
        strcpy(variable->name, source->property);
        variable->object = image->object;
        variable->context = image->context;
        variables[i].name = source->name;
        variables[i].value.state = variable;
        variables[i].value.load = read_variable;
        variables[i].value.store = source->writable ? write_variable : NULL;
    }
    return 1;
}


/* [GC:] EN/RU: See flashvm/styxrpc_module.h. / См. flashvm/styxrpc_module.h. */
FVM_Result fvm_styxrpc_object_image_create(FVM_Context *context, const FVM_StyxRpcObjectSpec *spec,
    FVM_StyxRpcObjectImage **out)
{
    StyxRpcModuleEntry entries[STYXRPC_MAX_EXPORTS];
    StyxRpcModuleVariable variables[STYXRPC_MAX_EXPORTS];
    StyxRpcModuleSpec module;
    FVM_StyxRpcObjectImage *image;
    FVM_Result status;
    size_t i;
    if (!context || !spec || !spec->object || !out || !spec->methods || !spec->method_count ||
        spec->method_count > STYXRPC_MAX_EXPORTS || spec->variable_count > STYXRPC_MAX_EXPORTS ||
        (spec->variable_count && !spec->variables))
    {
        return FVM_INVALID_ARGUMENT;
    }
    image = calloc(1, sizeof(*image));
    if (!image)
    {
        return FVM_OUT_OF_MEMORY;
    }
    image->context = context;
    image->object = spec->object;
    status = fvm_gc_pin(context, image->object);
    if (status != FVM_OK)
    {
        free(image);
        return status;
    }
    for (i = 0; status == FVM_OK && i < spec->method_count; ++i)
    {
        status = bind_method(image, i, spec->methods + i, entries + i);
    }
    module.name = spec->name;
    module.entries = entries;
    module.count = spec->method_count;
    module.variables = variables;
    module.variable_count = spec->variable_count;
    module.registry = spec->registry;
    module.control = spec->control;
    if (status == FVM_OK && (!bind_variables(image, spec, variables) ||
        styxrpc_module_create(&module, &image->module) != STYXRPC_OK))
    {
        status = FVM_INVALID_ARGUMENT;
    }
    if (status != FVM_OK)
    {
        fvm_styxrpc_object_image_destroy(image);
        return status;
    }
    *out = image;
    return FVM_OK;
}


/* EN/RU: See flashvm/styxrpc_module.h. / См. flashvm/styxrpc_module.h. */
StyxRpcModule *fvm_styxrpc_object_image_module(FVM_StyxRpcObjectImage *image)
{
    return image ? image->module : NULL;
}


/* [GC:] EN/RU: See flashvm/styxrpc_module.h. / См. flashvm/styxrpc_module.h. */
void fvm_styxrpc_object_image_destroy(FVM_StyxRpcObjectImage *image)
{
    if (image)
    {
        styxrpc_module_destroy(image->module);
        fvm_gc_unpin(image->context, image->object);
        free(image);
    }
}
