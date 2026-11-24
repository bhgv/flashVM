/* Copyright (c) 2026 bhgv
 * EN: Script admission of a checked object image on the same paired Styx session.
 * RU: Допуск образа AS на том же двустороннем соединении Styx.
 */
#include "player_services.h"
#include "session_methods.h"
#include "flashvm/styxrpc_module.h"
#include "flashvm/styxrpc_channel.h"
#include "flashvm/extensions.h"
#include "flashvm/gc_roots.h"
#include <stdlib.h>
#include <string.h>

struct FVM_StyxRpcPlayerServices
{
    FVM_StyxRpcSession session;    /* Owned script session and open launch tickets. */
    StyxRpcModuleSlot *slot;       /* Borrowed publication slot of this player's server. */
    FVM_StyxRpcObjectImage *image; /* Retained object and compiled module, at most one per movie. */
    int active;                    /* Movie admitted to publication. */
};


/* [GC:] EN: Parse one fixed-size scalar or managed method descriptor.
 * RU: Прочитать один описатель фиксированного размера для скалярного или управляемого метода. */
static FVM_Result method_descriptor(FVM_Context *context, const FVM_Value *descriptor,
    FVM_StyxRpcObjectMethod *method, char *name, char *member)
{
    static const char *const keys[] = {"0", "1", "2", "3", "4"};
    FVM_Value fields[5];
    FVM_Value length;
    FVM_GCRootScope *roots;
    uint64_t count, signature = 0;
    size_t i;
    FVM_Result status;
    if (descriptor->type != FVM_VALUE_OBJECT || !descriptor->object)
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = fvm_object_get(context, descriptor->object, "length", &length);
    if (status != FVM_OK)
    {
        return status;
    }
    if (!fvm_styxrpc_number(&length, 5, &count) || (count != 2 && count != 5))
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = fvm_gc_roots_begin(context, &roots);
    if (status != FVM_OK)
    {
        return status;
    }
    for (i = 0; status == FVM_OK && i < count; ++i)
    {
        int own;
        status = fvm_object_has_own(context, descriptor->object, keys[i], &own);
        if (status != FVM_OK || !own)
        {
            status = status != FVM_OK ? status : FVM_INVALID_ARGUMENT;
            break;
        }
        status = fvm_object_get(context, descriptor->object, keys[i], fields + i);
        if (status == FVM_OK)
        {
            status = fvm_gc_roots_add(roots, fields + i);
        }
    }
    if (status == FVM_OK &&
        (!fvm_styxrpc_name(fields, name, STYXRPC_TREE_NAME + 1) ||
         !fvm_styxrpc_name(fields + 1, member, STYXRPC_TREE_NAME + 1)))
    {
        status = FVM_INVALID_ARGUMENT;
    }
    if (status == FVM_OK && count == 5)
    {
        if (!fvm_styxrpc_number(fields + 2, UINT32_MAX, &signature) || !signature ||
            !fvm_styxrpc_name(fields + 3, method->schema.arguments, sizeof(method->schema.arguments)) ||
            !fvm_styxrpc_name(fields + 4, method->schema.result, sizeof(method->schema.result)))
        {
            status = FVM_INVALID_ARGUMENT;
        }
    }
    if (status == FVM_OK)
    {
        method->name = name;
        method->method = member;
        method->kind = count == 2 ? STYXRPC_MODULE_INT : STYXRPC_MODULE_MANAGED;
        method->signature = count == 2 ? STYXRPC_INT_SIGNATURE : (uint32_t)signature;
    }
    fvm_gc_roots_end(roots);
    return status;
}


/* [ASINC] [GC:] EN: Compile one typed image from an AS receiver and explicit method descriptors.
 * RU: Собрать типизированный образ из объекта AS и явных описателей его методов. */
static FVM_Result publish_object(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    FVM_StyxRpcPlayerServices *services = state;
    FVM_StyxRpcObjectMethod methods[STYXRPC_MAX_EXPORTS];
    char names[STYXRPC_MAX_EXPORTS][STYXRPC_TREE_NAME + 1];
    char members[STYXRPC_MAX_EXPORTS][STYXRPC_TREE_NAME + 1];
    char module[STYXRPC_TREE_NAME + 1];
    FVM_StyxRpcObjectSpec spec = {0};
    FVM_StyxRpcObjectImage *image = NULL;
    FVM_Result status;
    unsigned i;
    (void)self;
    if (!services->session.client || services->image || argc < 3 ||
        argc - 2 > STYXRPC_MAX_EXPORTS || args[1].type != FVM_VALUE_OBJECT ||
        !fvm_styxrpc_name(args, module, sizeof(module)) || module[0] == '.' || strchr(module, '/'))
    {
        return FVM_INVALID_ARGUMENT;
    }
    memset(methods, 0, sizeof(methods));
    for (i = 0; i < argc - 2; ++i)
    {
        status = method_descriptor(context, args + i + 2, methods + i, names[i], members[i]);
        if (status != FVM_OK)
        {
            return status;
        }
    }
    if (services->image)
    {
        return FVM_INVALID_ARGUMENT;
    }
    spec.name = module;
    spec.object = args[1].object;
    spec.methods = methods;
    spec.method_count = argc - 2;
    status = fvm_styxrpc_object_image_create(context, &spec, &image);
    if (status == FVM_OK && services->active &&
        styxrpc_module_slot_set(services->slot, fvm_styxrpc_object_image_module(image)) != STYXRPC_OK)
    {
        fvm_styxrpc_object_image_destroy(image);
        return FVM_ERROR;
    }
    if (status == FVM_OK)
    {
        services->image = image;
        fvm_value_boolean(out, 1);
    }
    return status;
}


/* [GC:] EN/RU: See player_services.h. / См. player_services.h. */
FVM_Result fvm_styxrpc_player_services(FVM_Context *context, struct StyxClient *client,
    StyxRpcModuleSlot *slot, FVM_StyxRpcPlayerServices **out)
{
    FVM_StyxRpcPlayerServices *services;
    FVM_Value object;
    FVM_GCRootScope *roots;
    FVM_Result status = fvm_extensions_install(context);
    if (status != FVM_OK)
    {
        return status;
    }
    services = calloc(1, sizeof(*services));
    if (!services)
    {
        return FVM_OUT_OF_MEMORY;
    }
    services->slot = slot;
    *out = services;
    status = fvm_gc_roots_begin(context, &roots);
    if (status != FVM_OK)
    {
        return status;
    }
    status = fvm_styxrpc_session_bind(context, &services->session, client, NULL, &object);
    if (status == FVM_OK)
    {
        status = fvm_gc_roots_add(roots, &object);
    }
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(context, object.object, "publishObject", publish_object, services, NULL);
    }
    if (status == FVM_OK)
    {
        status = fvm_set_global(context, "styxrpc", &object);
    }
    fvm_gc_roots_end(roots);
    if (status == FVM_OK)
    {
        status = fvm_styxrpc_channel_factory(context, "styxChannel");
    }
    return status;
}


/* EN/RU: See player_services.h. / См. player_services.h. */
FVM_Result fvm_styxrpc_player_activate(StyxRpcModuleSlot *slot, FVM_StyxRpcPlayerServices *services)
{
    StyxRpcModule *image = services && services->image ? fvm_styxrpc_object_image_module(services->image) : NULL;
    if (styxrpc_module_slot_set(slot, image) != STYXRPC_OK)
    {
        return FVM_ERROR;
    }
    if (services)
    {
        services->active = 1;
    }
    return FVM_OK;
}


/* EN/RU: See player_services.h. / См. player_services.h. */
void fvm_styxrpc_player_disarm(FVM_StyxRpcPlayerServices *services)
{
    if (services)
    {
        services->active = 0;
        fvm_styxrpc_session_close(&services->session);
        fvm_styxrpc_object_image_destroy(services->image);
        services->image = NULL;
    }
}
