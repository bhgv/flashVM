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
#include "disvm/refcount.h"
#include "sys_capture.h"
#include "scalar_owner_internal.h"
#include "raise.h"
#include "vfsnative_decl.h"
#include "../../native/terminal/scalar_verify.h"
#include <string.h>


/* EN: Return one owned local link for the supplied compiled import list.
 * RU: Вернуть свою локальную связь для списка компилированных импортов. */
static Modlink *link_owner(char *path, Import *imports)
{
    Module *module = dvm_namespace_read_imports(path, NULL, 1, imports);
    if (!module)
    {
        return H;
    }
    if (!module->prog || module->qid.type == DVM_MODULE_ADAPTER_QID)
    {
        unload(module);
        kwerrstr("scalar owner requires a local Dis implementation");
        return H;
    }
    return linkmod(module, imports, 1);
}


/* EN: Replace the caller's old result with one linked owner reference.
 * RU: Заменить прежний результат клиента одной ссылкой на связанный модуль. */
static void load_owner(char *path, Import *imports, Modlink **result)
{
    Modlink *empty = H;
    void *previous = dvm_sys_owner(result);
    memmove(result, &empty, sizeof(empty));
    destroy(previous);
    Modlink *link = link_owner(path, imports);
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


/* EN: Parse all export slots before linking. Mixed mode requires a width prefix.
 * RU: Разобрать все слоты до связывания. В смешанном режиме требуется префикс ширины. */
static void parse_exports(char *names, Import *imports, int mixed)
{
    char *name = names;
    for (size_t i = 0; name; i++)
    {
        char *separator = strchr(name, '\t');
        if (separator)
        {
            *separator = '\0';
        }
        uint32_t signature = DVM_SCALAR_INT_SIGNATURE;
        if (mixed)
        {
            if ((name[0] != 'i' && name[0] != 'b') || name[1] != ':')
            {
                error("invalid mixed owner export type");
            }
            signature = name[0] == 'b' ? DVM_SCALAR_BIG_SIGNATURE : DVM_SCALAR_INT_SIGNATURE;
            name += 2;
        }
        if (i == STYX_MOUNTED_EXPORT_LIMIT || !styxscalar_name_length(name))
        {
            error("invalid scalar owner export list");
        }
        imports[i] = (Import){(int32_t)signature, name};
        name = separator ? separator + 1 : NULL;
    }
}


/* EN: Bind eight or fewer int exports into one cloned data area.
 * RU: Связать до восьми экспортов int с одной областью данных. */
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
    parse_exports(names, imports, 0);
    load_owner(path, imports, call.ret);
}


/* EN: Publish two typed views of one compatible Modlink without cloning module data.
 * RU: Выдать два типизированных представления одной связи без копирования данных модуля. */
void Vfsnative_scalarmixedload(void *frame)
{
    F_Vfsnative_scalarmixedload call;
    Modlink *result[2] = {H, H};
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarmixedload, ret), sizeof(result));
    char path[DVM_NS_PATH_LIMIT], names[DVM_NS_PATH_LIMIT];
    if (!dvm_scalar_copy_path(call.path, path) || !dvm_scalar_copy_path(call.names, names))
    {
        error("scalar owner implementation unavailable");
    }
    Import imports[STYX_MOUNTED_EXPORT_LIMIT + 1] = {0};
    parse_exports(names, imports, 1);
    Modlink *previous[2];
    memmove(previous, call.ret, sizeof(previous));
    memmove(call.ret, result, sizeof(result));
    destroy(previous[0]);
    destroy(previous[1]);
    Modlink *link = link_owner(path, imports);
    if (link != H)
    {
        if (dvm_refcount_retain_u(&D2H(link)->ref) != DVM_REFCOUNT_LIVE)
        {
            destroy(link);
            error(exHeap);
        }
        Setmark(D2H(link));
        result[0] = result[1] = link;
        memmove(call.ret, result, sizeof(result));
    }
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
