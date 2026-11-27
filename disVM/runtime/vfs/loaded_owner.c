/* Copyright (c) 2026 bhgv
 * EN: Export the result of one ordinary Dis load to the local Styx dispatcher.
 * RU: Передача результата обычной загрузки Dis локальному диспетчеру Styx.
 */
#include "cap_internal.h"
#include "loader.h"
#include "module_shapes.h"
#include "module_counts.h"
#include "disvm/refcount.h"
#include "namespace.h"
#include "scalar_owner_internal.h"
#include "scalar_direct.h"
#include "managed_value.h"
#include "vfsnative_decl.h"
#include "../../native/terminal/load_profile.h"
#include "../../native/terminal/scalar_record.h"
#include "../../native/terminal/scalar_verify.h"

_Static_assert(sizeof(WORD) == sizeof(Modlink *), "NC64 typed load result cell");

enum
{
    VIEW_INT, /* fn(int): int. */
    VIEW_BIG, /* fn(big): big. */
    VIEW_TUPLE, /* One tuple argument and result. */
    VIEW_REF, /* One scalar-field ADT reference. */
    VIEW_LIST, /* Three arguments including a tuple. */
    VIEW_REF_PAIR, /* Two references with call-local identity. */
    VIEW_COUNT /* Number of typed return views. */
};

typedef struct LoadedOwner
{
    WORD tag; /* Request tag chosen from the actual compiled signature. */
    Modlink *views[VIEW_COUNT]; /* One rooted module view on success. */
} LoadedOwner;


typedef struct GatewayOwner
{
    LoadedOwner typed; /* Existing profile, or dynamic tag with empty typed views. */
    Modlink *raw; /* Owned scalar-only view when the shape selects a dynamic call. */
    WORD arguments; /* Flattened input cells, selects the one allowed slot. */
    WORD results; /* Flattened scalar result cells to publish. */
    Modlink *managed; /* Private bridge for the owning VM. */
} GatewayOwner;


/* EN: Map a recognized unscoped request tag to its typed Limbo view.
 * RU: Сопоставить известный тег запроса с типизированным представлением Limbo. */
static int view_slot(unsigned long tag)
{
    switch (tag)
    {
    case STYX_FUNCTION_REQUEST_TAG:
        return VIEW_INT;
    case STYX_FUNCTION_BIG_TAG:
        return VIEW_BIG;
    case STYX_TUPLE_REQUEST_TAG:
        return VIEW_TUPLE;
    case STYX_REF_REQUEST_TAG:
        return VIEW_REF;
    case STYX_CALL_LIST_REQUEST_TAG:
        return VIEW_LIST;
    case STYX_REF_PAIR_REQUEST_TAG:
        return VIEW_REF_PAIR;
    default:
        return -1;
    }
}


/* EN: Read one original module, returning its owned reference on success.
 * RU: Прочитать исходный модуль и вернуть собственную ссылку при успехе. */
static Module *read_original(char *path)
{
    Module *module = dvm_namespace_read_imports(path, NULL, 1, NULL);
    if (module && (!module->prog || module->qid.type == DVM_MODULE_ADAPTER_QID))
    {
        unload(module);
        return NULL;
    }
    return module;
}


/* EN: Check one compiled export against the owner gateway; update the
 * borrowed gateway selection only for an admitted shape.
 * RU: Проверить скомпилированный экспорт для шлюза владельца. Менять
 * заимствованное состояние шлюза только после допуска схемы. */
static unsigned long export_profile(Module *module, const Link *export, StyxCallShape *shape, GatewayOwner *gateway)
{
    if (!export->frame || !export->u.pc)
    {
        return 0;
    }
    uint32_t signature = (uint32_t)export->sig;
    dvm_module_export_shape(module, (size_t)(export - module->ext), shape);
    unsigned long tag = shape->arguments[0] ? styxshaperequest(signature, shape) : styxloadedrequest(signature);
    size_t words;
    unsigned char references;
    if (tag == STYX_DYNAMIC_REQUEST_TAG)
    {
        size_t leaves, result;
        if (!gateway || !styxshapecells(shape, &leaves, &result) ||
            !dvm_styx_frame_layout(shape->arguments, &words, &references) ||
            references || !dvm_styx_export_frame(export->frame, words, references))
        {
            return 0;
        }
        gateway->arguments = words;
        gateway->results = result;
    }
    else if (tag == STYX_REF_REQUEST_TAG || tag == STYX_REF_PAIR_REQUEST_TAG)
    {
        size_t count = tag == STYX_REF_REQUEST_TAG ? 1 : 2;
        if (!styxloadedrequest(signature) && (!gateway ||
            !dvm_styx_frame_layout(shape->arguments, &words, &references) ||
            words != count || !dvm_styx_export_frame(export->frame, words, references)))
        {
            return 0;
        }
        if (gateway)
        {
            size_t results = 2;
            styxshaperef(shape, &results);
            gateway->results = results;
        }
    }
    else if (tag == STYX_MANAGED_REQUEST_TAG &&
        (!gateway || !dvm_styx_managed_export_supported(export->frame, shape)))
    {
        return 0;
    }
    return tag;
}


/* EN: Bind a one-slot typed view, consuming module. When anchor is supplied,
 * the new view retains the same MP after normal linking and does no cloning.
 * RU: Связать однослотовое представление, поглотив module. При заданном anchor
 * представление удерживает тот же MP после обычного связывания без копирования. */
static Modlink *link_export(Module *module, Modlink *anchor, char *name, uint32_t *signature, StyxCallShape *shape, GatewayOwner *gateway)
{
    if (!module)
    {
        return H;
    }
    for (Link *export = module->ext; export && export->name; export++)
    {
        if (!strcmp(export->name, name))
        {
            unsigned long tag = export_profile(module, export, shape, gateway);
            if (!tag)
            {
                break;
            }
            *signature = (uint32_t)export->sig;
            size_t slot = tag == STYX_DYNAMIC_REQUEST_TAG ? (size_t)gateway->arguments : 0;
            Import imports[STYX_DYNAMIC_CELLS + 2] = {{0, NULL}};
            for (size_t i = 0; i <= slot; i++)
            {
                imports[i] = (Import){export->sig, name};
            }
            Modlink *link = linkmod(module, imports, anchor == H);
            if (link != H && slot)
            {
                /* Only the compiler-matched arity slot may be entered. */
                memset(link->links, 0, slot * sizeof(*link->links));
            }
            if (link != H && anchor != H && anchor->MP != H)
            {
                Heap *data = D2H(anchor->MP);
                if (dvm_refcount_retain_u(&data->ref) != DVM_REFCOUNT_LIVE)
                {
                    destroy(link);
                    error(exHeap);
                }
                Setmark(data);
                link->MP = anchor->MP;
            }
            return link;
        }
    }
    unload(module);
    return H;
}


/* EN: Reset the captured typed result and drop its previous rooted views.
 * RU: Очистить захваченный результат и снять ссылки с прежних представлений. */
static LoadedOwner reset_result(void *ret)
{
    LoadedOwner result = {0}, previous;
    memcpy(&previous, ret, sizeof(previous));
    for (size_t i = 0; i < VIEW_COUNT; i++)
    {
        result.views[i] = H;
    }
    memcpy(ret, &result, sizeof(result));
    for (size_t i = 0; i < VIEW_COUNT; i++)
    {
        destroy(previous.views[i]);
    }
    return result;
}


/* EN: Encode the exact export offer from the selected compiled identity.
 * RU: Закодировать предложение из выбранной компилированной сигнатуры. */
static int encode_offer(const char *name, uint32_t signature, const StyxCallShape *shape, unsigned char *bytes,
    size_t capacity, size_t *size)
{
    return shape->arguments[0] ? styxshapedoffer(name, signature, shape, bytes, capacity, size) :
        styxloadedoffer(name, signature, bytes, capacity, size);
}


/* EN: Send the staged profile or refusal to the private route, if supplied.
 * RU: Отправить профиль либо отказ в служебный маршрут, если он задан. */
static void send_offer(String *offer, unsigned char *bytes, size_t capacity, size_t size)
{
    if (offer == H)
    {
        return;
    }
    if (!size && !styxscalarwrite(STYX_FUNCTION_PROFILE_TAG, STYX_SCALAR_ERROR, 0, bytes, capacity, &size))
    {
        error("cannot encode loaded owner refusal");
    }
    dvm_scalar_owner_record(offer, bytes, size, 0);
}


/* EN: Root the accepted typed view in ret before publishing a complete
 * profile through offer. Consume link and preserve nil views on refusal.
 * RU: Укоренить принятое представление в ret до отправки полного профиля
 * через offer. Поглотить link, при отказе сохранить nil в результате. */
static void publish_view(void *ret, LoadedOwner result, String *offer, const char *name,
    Modlink *link, uint32_t signature, const StyxCallShape *shape)
{
    unsigned char bytes[STYX_SHAPED_OFFER_MAX];
    size_t size = 0;
    if (link != H)
    {
        unsigned long tag = shape->arguments[0] ? styxshaperequest(signature, shape) : styxloadedrequest(signature);
        int slot = view_slot(tag);
        int offered = encode_offer(name, signature, shape, bytes, sizeof(bytes), &size);
        if (slot >= 0 && offered)
        {
            result.tag = tag;
            Setmark(D2H(link));
            result.views[slot] = link;
            memcpy(ret, &result, sizeof(result));
        }
        else
        {
            destroy(link);
            size = 0;
        }
    }
    send_offer(offer, bytes, sizeof(bytes), size);
}


/* EN: Bind and publish a typed view using one original load for one export.
 * RU: Связать и опубликовать типизированное представление одного экспорта. */
void Vfsnative_scalarloaded(void *frame)
{
    F_Vfsnative_scalarloaded call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarloaded, ret), sizeof(LoadedOwner));
    char path[DVM_NS_PATH_LIMIT], name[DVM_NS_PATH_LIMIT];
    if (!dvm_scalar_copy_path(call.path, path) || !dvm_scalar_copy_path(call.exportname, name) ||
        !styxscalar_name_length(name))
    {
        error("loaded owner export unavailable");
    }
    LoadedOwner result = reset_result(call.ret);
    uint32_t signature = 0;
    StyxCallShape shape = {{0}, {0}};
    Modlink *link = link_export(read_original(path), H, name, &signature, &shape, NULL);
    publish_view(call.ret, result, call.offer, name, link, signature, &shape);
}


/* EN: Load, clone and root one original data instance for a set of exports.
 * RU: Загрузить, скопировать и укоренить один экземпляр данных для набора экспортов. */
void Vfsnative_scalarloadanchor(void *frame)
{
    F_Vfsnative_scalarloadanchor call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarloadanchor, ret), sizeof(*call.ret));
    char path[DVM_NS_PATH_LIMIT];
    if (!dvm_scalar_copy_path(call.path, path))
    {
        error("loaded owner module unavailable");
    }
    Modlink *previous = dvm_sys_owner(call.ret);
    *call.ret = H;
    destroy(previous);
    Import empty[] = {{0, NULL}};
    Modlink *anchor = linkmod(read_original(path), empty, 1);
    if (anchor != H)
    {
        Setmark(D2H(anchor));
        *call.ret = anchor;
    }
}


/* EN: Borrow the original Module on a rooted data anchor, excluding adapters.
 * RU: Заимствовать исходный Module укоренённого экземпляра без адаптеров. */
static Module *anchor_module(Modlink *anchor)
{
    Module *module = anchor != H && !anchor->nlinks ? anchor->m : NULL;
    return module && module->prog && module->qid.type != DVM_MODULE_ADAPTER_QID ? module : NULL;
}


/* EN: Retain the original Module belonging to the rooted anchor, for linking.
 * RU: Удержать исходный Module укоренённого экземпляра для связывания. */
static Module *retain_anchor(Modlink *anchor)
{
    Module *module = anchor_module(anchor);
    if (module)
    {
        dvm_runtime_module_retain(&module->ref);
    }
    return module;
}


#include "loaded_catalogue.inc"


/* EN: Bind a typed view of a rooted data instance and publish its profile.
 * RU: Связать типизированное представление укоренённого экземпляра и выдать его профиль. */
void Vfsnative_scalarloadedview(void *frame)
{
    F_Vfsnative_scalarloadedview call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarloadedview, ret), sizeof(LoadedOwner));
    char name[DVM_NS_PATH_LIMIT];
    if (!dvm_scalar_copy_path(call.exportname, name) || !styxscalar_name_length(name))
    {
        error("loaded owner export unavailable");
    }
    LoadedOwner result = reset_result(call.ret);
    Modlink *anchor = dvm_sys_owner(&call.anchor);
    uint32_t signature = 0;
    StyxCallShape shape = {{0}, {0}};
    Modlink *link = link_export(retain_anchor(anchor), anchor, name, &signature, &shape, NULL);
    publish_view(call.ret, result, call.offer, name, link, signature, &shape);
}



/* EN: Publish one exact compiled export through the shared owner dispatcher.
 * RU: Опубликовать фактический скомпилированный экспорт через общий диспетчер. */
void Vfsnative_scalarloadedslot(void *frame)
{
    F_Vfsnative_scalarloadedslot call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarloadedslot, ret), sizeof(GatewayOwner));
    char name[DVM_NS_PATH_LIMIT];
    if (!dvm_scalar_copy_path(call.exportname, name) || !styxscalar_name_length(name))
    {
        error("loaded owner export unavailable");
    }
    GatewayOwner result = {.raw = H, .managed = H};
    unsigned char *destination = (unsigned char *)call.ret;
    Modlink *previous = dvm_sys_owner(destination + offsetof(GatewayOwner, raw));
    memcpy(destination + offsetof(GatewayOwner, raw), &result.raw, sizeof(result.raw));
    destroy(previous);
    previous = dvm_sys_owner(destination + offsetof(GatewayOwner, managed));
    memcpy(destination + offsetof(GatewayOwner, managed), &result.managed, sizeof(result.managed));
    destroy(previous);
    result.typed = reset_result(destination);
    memcpy(destination, &result, sizeof(result));
    Modlink *anchor = dvm_sys_owner(&call.anchor);
    uint32_t signature = 0;
    StyxCallShape shape = {{0}, {0}};
    Modlink *link = link_export(retain_anchor(anchor), anchor, name, &signature, &shape, &result);
    unsigned long tag = styxshaperequest(signature, &shape);
    if (link != H && tag == STYX_MANAGED_REQUEST_TAG)
    {
        Setmark(D2H(link));
        result.managed = link;
        memcpy(destination, &result, sizeof(result));
        result.managed = dvm_styx_managed_gateway(link, &shape);
        if (result.managed != H)
        {
            Setmark(D2H(result.managed));
        }
        memcpy(destination, &result, sizeof(result));
        destroy(link);
        unsigned char bytes[STYX_SHAPED_OFFER_MAX];
        size_t size = 0;
        if (result.managed != H)
        {
            if (encode_offer(name, signature, &shape, bytes, sizeof(bytes), &size))
            {
                result.typed.tag = tag;
            }
            else
            {
                destroy(result.managed);
                result.managed = H;
            }
            memcpy(destination, &result, sizeof(result));
        }
        send_offer(call.offer, bytes, sizeof(bytes), size);
        return;
    }
    if (link == H || tag != STYX_DYNAMIC_REQUEST_TAG)
    {
        memcpy(destination, &result, sizeof(result));
        publish_view(destination, result.typed, call.offer, name, link, signature, &shape);
        return;
    }
    unsigned char bytes[STYX_SHAPED_OFFER_MAX];
    size_t size = 0;
    if (encode_offer(name, signature, &shape, bytes, sizeof(bytes), &size))
    {
        result.typed.tag = STYX_DYNAMIC_REQUEST_TAG;
        Setmark(D2H(link));
        result.raw = link;
        memcpy(destination, &result, sizeof(result));
    }
    else
    {
        destroy(link);
        size = 0;
    }
    send_offer(call.offer, bytes, sizeof(bytes), size);
}
