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


/* EN: Consume the namespace load exactly once and link the selected actual
 * export. On success return its owned link and copy the signature to signature.
 * RU: Один раз загрузить модуль в namespace и связать фактический экспорт.
 * При успехе вернуть свою связь и записать сигнатуру в signature. */
static Modlink *load_export(char *path, char *name, uint32_t *signature)
{
    Module *module = dvm_namespace_read_imports(path, NULL, 1, NULL);
    if (!module)
    {
        return H;
    }
    if (module->prog && module->qid.type != DVM_MODULE_ADAPTER_QID)
    {
        for (Link *export = module->ext; export && export->name; export++)
        {
            if (!strcmp(export->name, name) && export->frame && export->u.pc &&
                styxloadedrequest((uint32_t)export->sig))
            {
                Import imports[] = {{export->sig, name}, {0, NULL}};
                *signature = (uint32_t)export->sig;
                return linkmod(module, imports, 1);
            }
        }
    }
    unload(module);
    return H;
}


/* EN: Return one typed view rooted in the caller frame before offering its
 * exact export to the dispatcher. Only the copied name/signature reach Styx.
 * RU: Укоренить одно типизированное представление в кадре до предложения
 * точного экспорта диспетчеру. Через Styx идут только скопированные имя и сигнатура. */
void Vfsnative_scalarloaded(void *frame)
{
    F_Vfsnative_scalarloaded call;
    LoadedOwner result = {0}, previous;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarloaded, ret), sizeof(result));
    char path[DVM_NS_PATH_LIMIT], name[DVM_NS_PATH_LIMIT];
    if (!dvm_scalar_copy_path(call.path, path) || !dvm_scalar_copy_path(call.exportname, name) ||
        !styxscalar_name_length(name))
    {
        error("loaded owner export unavailable");
    }
    memcpy(&previous, call.ret, sizeof(previous));
    for (size_t i = 0; i < VIEW_COUNT; i++)
    {
        result.views[i] = H;
    }
    memcpy(call.ret, &result, sizeof(result));
    for (size_t i = 0; i < VIEW_COUNT; i++)
    {
        destroy(previous.views[i]);
    }
    uint32_t signature = 0;
    Modlink *link = load_export(path, name, &signature);
    unsigned char bytes[STYX_SCALAR_INTERFACE_CAPACITY];
    size_t size;
    if (link != H)
    {
        result.tag = styxloadedrequest(signature);
        int slot = view_slot(result.tag);
        if (slot < 0 || !styxloadedoffer(name, signature, bytes, sizeof(bytes), &size))
        {
            destroy(link);
            error("invalid loaded owner profile");
        }
        Setmark(D2H(link));
        result.views[slot] = link;
        memcpy(call.ret, &result, sizeof(result));
    }
    else if (!styxscalarwrite(STYX_FUNCTION_PROFILE_TAG, STYX_SCALAR_ERROR, 0, bytes, sizeof(bytes), &size))
    {
        error("cannot encode loaded owner refusal");
    }
    dvm_scalar_owner_record(call.offer, bytes, size);
}
