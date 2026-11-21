/* Copyright (c) 2026 bhgv
 * EN: Bind owner exports by compiled signatures and marshal scalar reply frames.
 * RU: Связывать экспорты владельца по сигнатурам и читать кадры скалярных ответов.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "loader.h"
#include "namespace.h"
#include "kernel.h"
#include "sys_capture.h"
#include "scalar_owner_internal.h"
#include "raise.h"
#include "vfsnative_decl.h"
#include "../../native/terminal/scalar_verify.h"
#include <string.h>


/* [GC:] EN/RU: Bind requested scalar exports on one ordinary local Modlink.
 * Связать запрошенные скалярные экспорты в одном обычном локальном Modlink. */
static void load_owner(char *path, Import *imports, Modlink **result)
{
    Modlink *empty = H;
    void *previous = dvm_sys_owner(result);
    memmove(result, &empty, sizeof(empty));
    destroy(previous);
    Module *module = dvm_namespace_read_imports(path, NULL, 1, imports);
    if (!module)
    {
        return;
    }
    if (!module->prog || module->qid.type == DVM_MODULE_ADAPTER_QID)
    {
        unload(module);
        kwerrstr("scalar owner requires a local Dis implementation");
        return;
    }
    Modlink *link = linkmod(module, imports, 1);
    if (link != H)
    {
        Setmark(D2H(link));
        memmove(result, &link, sizeof(link));
    }
}


/* EN/RU: Load the existing one-slot owner ABI into the caller's result.
 * Загрузить прежний однослотовый ABI владельца в результат вызова. */
void Vfsnative_scalarload(void *frame)
{
    F_Vfsnative_scalarload call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarload, ret), sizeof(*call.ret));
    char path[DVM_NS_PATH_LIMIT], name[DVM_NS_PATH_LIMIT];
    if (!dvm_scalar_copy_path(call.path, path) || !dvm_scalar_copy_path(call.exportname, name) || strlen(name) > INTERFACE_NAME_LIMIT)
    {
        error("scalar owner implementation unavailable");
    }
    Import imports[] = {{(int32_t)DVM_SCALAR_INT_SIGNATURE, name}, {0, NULL}};
    load_owner(path, imports, call.ret);
}


/* EN/RU: Bind up to eight names in request order, sharing one cloned data area.
 * Связать до восьми имён в порядке запроса с одной копией данных модуля. */
void Vfsnative_scalarloadset(void *frame)
{
    F_Vfsnative_scalarloadset call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarloadset, ret), sizeof(*call.ret));
    char path[DVM_NS_PATH_LIMIT], names[DVM_NS_PATH_LIMIT];
    if (!dvm_scalar_copy_path(call.path, path) || !dvm_scalar_copy_path(call.names, names))
    {
        error("scalar owner implementation unavailable");
    }
    Import imports[STYX_MOUNTED_EXPORT_LIMIT + 1] = {0};
    char *name = names;
    for (size_t i = 0; name; i++)
    {
        char *separator = strchr(name, '\t');
        if (separator)
        {
            *separator = '\0';
        }
        if (i == STYX_MOUNTED_EXPORT_LIMIT || !styxscalar_name_length(name))
        {
            error("invalid scalar owner export list");
        }
        imports[i] = (Import){(int32_t)DVM_SCALAR_INT_SIGNATURE, name};
        name = separator ? separator + 1 : NULL;
    }
    load_owner(path, imports, call.ret);
}



/* EN: Bind a single original Dis export with the compiled big signature.
 * RU: Связать исходный экспорт Dis с компилированной сигнатурой big. */
void Vfsnative_scalarbigload(void *frame)
{
    F_Vfsnative_scalarbigload call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarbigload, ret), sizeof(*call.ret));
    char path[DVM_NS_PATH_LIMIT], name[DVM_NS_PATH_LIMIT];
    if (!dvm_scalar_copy_path(call.path, path) || !dvm_scalar_copy_path(call.exportname, name) ||
        !styxscalar_name_length(name))
    {
        error("scalar owner implementation unavailable");
    }
    Import imports[] = {{(int32_t)DVM_SCALAR_BIG_SIGNATURE, name}, {0, NULL}};
    load_owner(path, imports, call.ret);
}


/* EN: Copy and deliver the legacy signed result frame.
 * RU: Скопировать и отправить прежний кадр знакового результата. */
void Vfsnative_scalarreply(void *frame)
{
    F_Vfsnative_scalarreply call;
    if (!frame || frame == H || (uintptr_t)frame > UINTPTR_MAX - sizeof(call))
    {
        error(exBounds);
    }
    memmove(&call, frame, sizeof(call));
    dvm_scalar_owner_reply(call.path, call.tag, call.success, call.value);
}


/* EN: Copy and deliver the full-width signed result frame.
 * RU: Скопировать и отправить полноразрядный кадр результата. */
void Vfsnative_scalarbigreply(void *frame)
{
    F_Vfsnative_scalarbigreply call;
    if (!frame || frame == H || (uintptr_t)frame > UINTPTR_MAX - sizeof(call))
    {
        error(exBounds);
    }
    memmove(&call, frame, sizeof(call));
    dvm_scalar_owner_reply(call.path, call.tag, call.success, call.value);
}
