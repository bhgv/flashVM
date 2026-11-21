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
static Modlink *link_export(Module *module, Modlink *anchor, char *name, uint32_t *signature, StyxCallShape *shape)
{
    if (!module)
    {
        return H;
    }
    for (Link *export = module->ext; export && export->name; export++)
    {
        if (!strcmp(export->name, name) && export->frame && export->u.pc && styxloadedrequest((uint32_t)export->sig))
        {
            Import imports[] = {{export->sig, name}, {0, NULL}};
            *signature = (uint32_t)export->sig;
            dvm_module_export_shape(module, (size_t)(export - module->ext), shape);
            Modlink *link = linkmod(module, imports, anchor == H);
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
        unsigned long tag = styxloadedrequest(signature);
        int slot = view_slot(tag);
        int offered = shape->arguments[0] ?
            styxshapedoffer(name, signature, shape, bytes, sizeof(bytes), &size) :
            styxloadedoffer(name, signature, bytes, sizeof(bytes), &size);
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
    if (!size && !styxscalarwrite(STYX_FUNCTION_PROFILE_TAG, STYX_SCALAR_ERROR, 0, bytes, sizeof(bytes), &size))
    {
        error("cannot encode loaded owner refusal");
    }
    dvm_scalar_owner_record(offer, bytes, size);
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
    Modlink *link = link_export(read_original(path), H, name, &signature, &shape);
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
    Module *module = anchor != H && !anchor->nlinks ? anchor->m : NULL;
    if (module && (!module->prog || module->qid.type == DVM_MODULE_ADAPTER_QID))
    {
        module = NULL;
    }
    if (module)
    {
        dvm_runtime_module_retain(&module->ref);
    }
    uint32_t signature = 0;
    StyxCallShape shape = {{0}, {0}};
    Modlink *link = link_export(module, anchor, name, &signature, &shape);
    publish_view(call.ret, result, call.offer, name, link, signature, &shape);
}
