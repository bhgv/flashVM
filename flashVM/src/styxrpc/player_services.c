/* Copyright (c) 2026 bhgv
 * EN: Script admission of a checked object image on the same paired Styx session.
 * RU: Допуск образа AS на том же двустороннем соединении Styx.
 */
#include "player_services.h"
#include "session_methods.h"
#include "session_nodes.h"
#include "session_publish_messages.h"
#include "styxrpc/nodes.h"
#include "flashvm/styxrpc_module.h"
#include "flashvm/styxrpc_channel.h"
#include "flashvm/extensions.h"
#include "flashvm/gc_roots.h"
#include "../../ports/network_port.h"
#include <stdlib.h>
#include <string.h>

#define PLAYER_CHANNEL_LEASES 16U /* Maximum live result-channel grants per movie. */

struct FVM_StyxRpcPlayerServices
{
    FVM_StyxRpcSession session;    /* Owned script session and open launch tickets. */
    StyxRpcModuleSlot *slot;       /* Borrowed publication slot of this player's server. */
    FVM_StyxRpcPublishedMessages published; /* Owned script exchange registrations. */
    FVM_StyxRpcObjectImage *image; /* Retained object and compiled module, at most one per movie. */
    StyxRpcModuleChannels channels; /* Lease registry borrowed by the active published image. */
    StyxRpcModuleChannel channel_leases[PLAYER_CHANNEL_LEASES]; /* Owner-thread lease storage. */
    int active;                    /* Movie admitted to publication. */
};


typedef struct ObjectDescriptors
{
    FVM_StyxRpcObjectMethod methods[STYXRPC_MAX_EXPORTS];      /* Copied method contracts. */
    FVM_StyxRpcObjectVariable variables[STYXRPC_MAX_EXPORTS];  /* Copied variable contracts. */
    char names[2 * STYXRPC_MAX_EXPORTS][STYXRPC_TREE_NAME + 1];   /* Wire names. */
    char members[2 * STYXRPC_MAX_EXPORTS][STYXRPC_TREE_NAME + 1]; /* Receiver member names. */
    size_t method_count;   /* Initialized method contracts. */
    size_t variable_count; /* Initialized variable contracts. */
    int has_channels;      /* At least one method exports a leased result channel. */
} ObjectDescriptors;


/* EN: Append one fully decoded variable, retaining names borrowed from descriptors.
 * RU: Добавить одну декодированную переменную, заимствуя имена из descriptors. */
static int append_variable(ObjectDescriptors *descriptors, const FVM_Value *fields,
    const char *name, const char *member)
{
    FVM_StyxRpcObjectVariable *variable;
    char marker[4], mode[3];
    if (descriptors->variable_count == STYXRPC_MAX_EXPORTS ||
        !fvm_styxrpc_name(fields + 2, marker, sizeof(marker)) || strcmp(marker, "var") ||
        !fvm_styxrpc_name(fields + 3, mode, sizeof(mode)) || (strcmp(mode, "ro") && strcmp(mode, "rw")))
    {
        return 0;
    }
    variable = descriptors->variables + descriptors->variable_count++;
    variable->name = name;
    variable->property = member;
    variable->writable = !strcmp(mode, "rw");
    return 1;
}


/* EN: Append a scalar, shaped or result-channel contract and borrow its copied names.
 * RU: Добавить скалярный, типизированный или канальный контракт и заимствовать копии имён. */
static int append_method(ObjectDescriptors *descriptors, uint64_t count, const FVM_Value *fields,
    const char *name, const char *member)
{
    FVM_StyxRpcObjectMethod *method;
    uint64_t signature = STYXRPC_INT_SIGNATURE, slots = 0;
    char profile[8];
    if (descriptors->method_count == STYXRPC_MAX_EXPORTS || (count != 2 && (count < 5 || count > 7)))
    {
        return 0;
    }
    method = descriptors->methods + descriptors->method_count;
    if ((count >= 6 && (!fvm_styxrpc_name(fields + 5, profile, sizeof(profile)) ||
         strcmp(profile, count == 6 ? "graph" : "channel"))) ||
        (count == 7 && !fvm_styxrpc_number(fields + 6, FVM_STYXRPC_CHANNEL_QUEUE_SLOTS, &slots)))
    {
        return 0;
    }
    if (count > 2 &&
        (!fvm_styxrpc_number(fields + 2, UINT32_MAX, &signature) || !signature ||
         !fvm_styxrpc_name(fields + 3, method->schema.arguments, sizeof(method->schema.arguments)) ||
         !fvm_styxrpc_name(fields + 4, method->schema.result, sizeof(method->schema.result))))
    {
        return 0;
    }
    method->name = name;
    method->method = member;
    switch (count)
    {
    case 2:
        method->kind = STYXRPC_MODULE_INT;
        break;
    case 5:
        method->kind = STYXRPC_MODULE_MANAGED;
        break;
    case 6:
        method->kind = STYXRPC_MODULE_GRAPH;
        break;
    default:
        method->kind = STYXRPC_MODULE_CHANNEL;
        method->slots = (unsigned)slots;
        descriptors->has_channels = 1;
        break;
    }
    method->signature = (uint32_t)signature;
    ++descriptors->method_count;
    return 1;
}


/* [GC:] EN: Decode a method or variable into its own slot, pinning AS field values while inspecting them.
 * RU: Декодировать метод или переменную в её слот, удерживая поля AS на время проверки. */
static FVM_Result object_descriptor(FVM_Context *context, const FVM_Value *descriptor,
    ObjectDescriptors *descriptors, unsigned index)
{
    FVM_Value fields[7];
    FVM_GCRootScope *roots;
    uint64_t count;
    char *name = descriptors->names[index];
    char *member = descriptors->members[index];
    FVM_Result status = fvm_gc_roots_begin(context, &roots);
    if (status != FVM_OK)
    {
        return status;
    }
    status = fvm_styxrpc_fields(context, roots, descriptor, fields, 7, &count);
    if (status == FVM_OK &&
        (count < 2 || !fvm_styxrpc_name(fields, name, STYXRPC_TREE_NAME + 1) ||
         !fvm_styxrpc_name(fields + 1, member, STYXRPC_TREE_NAME + 1) ||
         !(count == 4 ? append_variable(descriptors, fields, name, member) :
             append_method(descriptors, count, fields, name, member))))
    {
        status = FVM_INVALID_ARGUMENT;
    }
    fvm_gc_roots_end(roots);
    return status;
}


/* EN: Fill the borrowed token buffer with cryptographically secure OS randomness.
 * RU: Заполнить заимствованный буфер токена криптографически стойкими байтами ОС. */
static int channel_entropy(void *unused, void *bytes, size_t size)
{
    (void)unused;
    return fvm_port_secure_random(bytes, size);
}


/* [ASINC] [GC:] EN: Compile one typed image from an AS receiver and explicit method descriptors.
 * RU: Собрать типизированный образ из объекта AS и явных описателей его методов. */
static FVM_Result publish_object(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    FVM_StyxRpcPlayerServices *services = state;
    ObjectDescriptors descriptors = {0};
    char module[STYXRPC_TREE_NAME + 1];
    FVM_StyxRpcObjectSpec spec = {0};
    FVM_StyxRpcObjectImage *image = NULL;
    FVM_Result status;
    unsigned i;
    (void)self;
    if (!services->session.client || services->image || argc < 3 ||
        argc - 2 > 2 * STYXRPC_MAX_EXPORTS || args[1].type != FVM_VALUE_OBJECT ||
        !fvm_styxrpc_name(args, module, sizeof(module)) || module[0] == '.' || strchr(module, '/'))
    {
        return FVM_INVALID_ARGUMENT;
    }
    for (i = 0; i < argc - 2; ++i)
    {
        status = object_descriptor(context, args + i + 2, &descriptors, i);
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
    spec.methods = descriptors.methods;
    spec.method_count = descriptors.method_count;
    spec.variables = descriptors.variables;
    spec.variable_count = descriptors.variable_count;
    if (descriptors.has_channels)
    {
        if (!styxrpc_module_channels_init(&services->channels, services->channel_leases,
            PLAYER_CHANNEL_LEASES, channel_entropy, NULL))
        {
            return FVM_ERROR;
        }
        spec.registry = &services->channels;
    }
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


/* [GC:] EN: Publish an AS function on the active movie's already attached session.
 * RU: Опубликовать функцию AS в уже подключённой сессии текущего фильма. */
static FVM_Result new_node(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    FVM_StyxRpcPlayerServices *services = state;
    (void)self;
    if (!services->active || !services->slot || !services->session.client)
    {
        return FVM_UNSUPPORTED;
    }
    return fvm_styxrpc_publish_node(context, services->slot, argc, args, out);
}


/* [GC:] EN: Withdraw one local publication in the active movie's attached session.
 * RU: Отозвать одну локальную публикацию в подключённой сессии текущего фильма. */
static FVM_Result remove_node(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    FVM_StyxRpcPlayerServices *services = state;
    char path[STYXRPC_NODE_PATH_MAX + 2];
    (void)context;
    (void)self;
    if (!services->active || !services->slot || !services->session.client)
    {
        return FVM_UNSUPPORTED;
    }
    if (argc != 1 || !fvm_styxrpc_name(args, path, sizeof(path)))
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_boolean(out, styxrpc_module_slot_remove_node(services->slot, path) == STYXRPC_OK);
    return FVM_OK;
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
        status = fvm_object_define_method_rooted(context, object.object, "newnode", new_node, services, NULL);
    }
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(context, object.object, "rmnode", remove_node, services, NULL);
    }
    if (status == FVM_OK)
    {
        status = fvm_set_global(context, "styxrpc", &object);
    }
    if (status == FVM_OK)
    {
        status = fvm_set_global(context, "vfs", &object);
    }
    fvm_gc_roots_end(roots);
    if (status == FVM_OK)
    {
        status = fvm_styxrpc_channel_factory(context, "styxChannel");
    }
    return status;
}


/* EN/RU: See player_services.h. / См. player_services.h. */
FVM_StyxRpcSession *fvm_styxrpc_player_binding(FVM_StyxRpcPlayerServices *services)
{
    return &services->session;
}


/* EN/RU: See player_services.h. / См. player_services.h. */
FVM_Result fvm_styxrpc_player_enable_messages(FVM_Context *context,
    FVM_StyxRpcPlayerServices *services, StyxRpcServer *server)
{
    return fvm_styxrpc_publish_messages_install(context, &services->published, server);
}


/* EN/RU: See player_services.h. / См. player_services.h. */
void fvm_styxrpc_player_clear_messages(FVM_StyxRpcPlayerServices *services)
{
    if (services)
    {
        fvm_styxrpc_publish_messages_clear(&services->published);
    }
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
        if (services->active)
        {
            styxrpc_module_slot_clear_nodes(services->slot);
        }
        services->active = 0;
        fvm_styxrpc_player_clear_messages(services);
        fvm_styxrpc_session_close(&services->session);
        fvm_styxrpc_object_image_destroy(services->image);
        services->image = NULL;
    }
}
