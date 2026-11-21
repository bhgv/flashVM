/* Copyright (c) 2026 bhgv
 * EN: Export the result of one ordinary Dis load to the local Styx dispatcher.
 * RU: Передача результата обычной загрузки Dis локальному диспетчеру Styx.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "loader.h"
#include "module_shapes.h"
#include "module_counts.h"
#include "disvm/refcount.h"
#include "namespace.h"
#include "sys_capture.h"
#include "scalar_owner_internal.h"
#include "vfsnative_decl.h"
#include "raise.h"
#include "../../native/terminal/load_profile.h"
#include "../../native/terminal/call_record.h"
#include "../../native/terminal/scalar_record.h"
#include "../../native/terminal/scalar_verify.h"
#include <string.h>

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
} GatewayOwner;


/* EN: Verify that the original export reserves all incoming scalar words and
 * that the pointer bits match the expected scalar-only or ref-only layout.
 * RU: Проверить размер кадра и соответствие карты раскладке скаляров или ref. */
static int gateway_frame(const Type *frame, size_t arguments, int references)
{
    const size_t first_cell = 8;
    if (frame->size < 0 || (size_t)frame->size < (first_cell + arguments) * sizeof(WORD) || frame->np < 0)
    {
        return 0;
    }
    for (size_t i = first_cell; i < first_cell + arguments; i++)
    {
        int pointer = i / 8 < (size_t)frame->np && (frame->map[i / 8] & (0x80 >> (i % 8)));
        if (pointer != references)
        {
            return 0;
        }
    }
    return 1;
}


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
        if (!strcmp(export->name, name) && export->frame && export->u.pc)
        {
            size_t slot = 0;
            *signature = (uint32_t)export->sig;
            dvm_module_export_shape(module, (size_t)(export - module->ext), shape);
            unsigned long tag = shape->arguments[0] ? styxshaperequest(*signature, shape) : styxloadedrequest(*signature);
            if (!tag)
            {
                break;
            }
            if (tag == STYX_DYNAMIC_REQUEST_TAG)
            {
                size_t leaves, result;
                StyxScalarLayout layout;
                if (!gateway || !styxshapecells(shape, &leaves, &result) ||
                    !styxscalar_layout(shape->arguments, &layout))
                {
                    break;
                }
                slot = (layout.size + sizeof(WORD) - 1) / sizeof(WORD);
                if (!gateway_frame(export->frame, slot, 0))
                {
                    break;
                }
                gateway->arguments = slot;
                gateway->results = result;
            }
            else if (tag == STYX_REF_REQUEST_TAG || tag == STYX_REF_PAIR_REQUEST_TAG)
            {
                size_t refs = tag == STYX_REF_REQUEST_TAG ? 1 : 2;
                if (!styxloadedrequest(*signature) && (!gateway || !gateway_frame(export->frame, refs, 1)))
                {
                    break;
                }
                if (gateway)
                {
                    size_t results = 2;
                    styxshaperef(shape, &results);
                    gateway->results = results;
                }
            }
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


/* EN: Send the staged profile or a refusal through the captured private route.
 * RU: Отправить профиль либо отказ через захваченный служебный маршрут. */
static void send_offer(String *offer, unsigned char *bytes, size_t capacity, size_t size)
{
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


/* EN: Retain the original Module belonging to a rooted data anchor.
 * RU: Удержать исходный Module укоренённого экземпляра данных. */
static Module *retain_anchor(Modlink *anchor)
{
    Module *module = anchor != H && !anchor->nlinks ? anchor->m : NULL;
    if (module && (!module->prog || module->qid.type == DVM_MODULE_ADAPTER_QID))
    {
        return NULL;
    }
    if (module)
    {
        dvm_runtime_module_retain(&module->ref);
    }
    return module;
}


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
    GatewayOwner result = {.raw = H};
    unsigned char *destination = (unsigned char *)call.ret;
    Modlink *previous = dvm_sys_owner(destination + offsetof(GatewayOwner, raw));
    memcpy(destination + offsetof(GatewayOwner, raw), &result.raw, sizeof(result.raw));
    destroy(previous);
    result.typed = reset_result(destination);
    memcpy(destination, &result, sizeof(result));
    Modlink *anchor = dvm_sys_owner(&call.anchor);
    uint32_t signature = 0;
    StyxCallShape shape = {{0}, {0}};
    Modlink *link = link_export(retain_anchor(anchor), anchor, name, &signature, &shape, &result);
    if (link == H || styxshaperequest(signature, &shape) != STYX_DYNAMIC_REQUEST_TAG)
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
