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
#include "../../native/terminal/scalar_record.h"
#include "../../native/terminal/tuple_record.h"
#include "../../native/terminal/call_record.h"
#include "../../native/terminal/ref_pair_record.h"
#include "raise.h"
#include "vfsnative_decl.h"
#include "../../native/terminal/scalar_verify.h"
#include <string.h>


extern char *dvm_runtime_file_path(String *name); /* Borrowed path while the calling frame is rooted. */


/* EN/RU: Copy a validated guest spelling before releasing managed admission.
 * Скопировать проверенный путь гостя до выхода из допуска VM. */
int dvm_scalar_copy_path(String *name, char output[DVM_NS_PATH_LIMIT])
{
    char *path = dvm_runtime_file_path(name);
    if (!path || !*path || strlen(path) >= DVM_NS_PATH_LIMIT)
    {
        return 0;
    }
    strcpy(output, path);
    return 1;
}


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



/* EN: Encode a scalar reply and use the common owner mailbox sender.
 * RU: Закодировать скалярный ответ и использовать общий приёмник. */
void dvm_scalar_owner_reply(String *name, int64_t tag, WORD success, int64_t value)
{
    unsigned char bytes[STYX_SCALAR_RECORD_CAPACITY];
    size_t size;
    if (tag <= 0 || tag > UINT32_MAX || !styxscalarwrite((unsigned long)tag,
            success == 1 ? STYX_SCALAR_VALUE : STYX_SCALAR_ERROR, value, bytes, sizeof(bytes), &size))
    {
        error("invalid scalar reply");
    }
    dvm_scalar_owner_record(name, bytes, size, 0);
}


/* EN: Bind the chosen named export to one slot with the required signature.
 * RU: Связать выбранный именованный экспорт с одним слотом по сигнатуре. */
void dvm_scalar_load_composed(String *path_input, String *name_input, uint32_t signature, Modlink **ret)
{
    char path[DVM_NS_PATH_LIMIT], name[DVM_NS_PATH_LIMIT];
    if (!dvm_scalar_copy_path(path_input, path) || !dvm_scalar_copy_path(name_input, name) ||
        !styxscalar_name_length(name))
    {
        error(signature == DVM_CALL_LIST_SIGNATURE ? "argument-list owner implementation unavailable" :
            (signature == DVM_REF_SIGNATURE || signature == DVM_REF_PAIR_SIGNATURE) ? "ref owner implementation unavailable" :
            "tuple owner implementation unavailable");
    }
    Import imports[] = {{(int32_t)signature, name}, {0, NULL}};
    load_owner(path, imports, ret);
}


/* EN: Link a local owner export with the compiler's exact tuple signature.
 * RU: Связать локальный экспорт по точной сигнатуре кортежа. */
void Vfsnative_scalartupleload(void *frame)
{
    F_Vfsnative_scalartupleload call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalartupleload, ret), sizeof(*call.ret));
    dvm_scalar_load_composed(call.path, call.exportname, DVM_TUPLE_SIGNATURE, call.ret);
}


/* EN: Link a local owner export with the exact scalar-field ref signature.
 * RU: Связать локальный экспорт по точной сигнатуре скалярного ref. */
void Vfsnative_scalarrefload(void *frame)
{
    F_Vfsnative_scalarrefload call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarrefload, ret), sizeof(*call.ret));
    dvm_scalar_load_composed(call.path, call.exportname, DVM_REF_SIGNATURE, call.ret);
}


/* EN: Decode one complete tuple in the tag selected by the owner service.
 * RU: Прочитать полный кортеж с тегом, выбранным сервисом владельца. */
static int decode_tuple(unsigned long tag, const void *bytes, size_t size, void *result)
{
    uint32_t pid;
    return styxtupleread(tag, bytes, size, &pid, result);
}


/* EN: Publish readiness and two tuple fields only after a complete read.
 * RU: Выдать признак готовности и два поля только после полного чтения. */
static void tuple_next(String *path, unsigned long tag, void *ret, int scoped)
{
    StyxTuple tuple = {0};
    int ready = dvm_scalar_owner_next(path, tag, decode_tuple, &tuple, scoped);
    const WORD cells[] = {ready, tuple.narrow, tuple.wide};
    dvm_sys_scalar_result(cells, ret, sizeof(cells));
}


/* EN: Read one snapshot of the client's scalar fields or return clean EOF.
 * RU: Прочитать один снимок полей клиента либо вернуть EOF. */
void Vfsnative_scalarrefnext(void *frame)
{
    F_Vfsnative_scalarrefnext call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarrefnext, ret), 3 * sizeof(WORD));
    tuple_next(call.path, STYX_REF_REQUEST_TAG, call.ret, 1);
}


/* EN: Read one original-Dis tuple request or return clean EOF.
 * RU: Прочитать один запрос кортежа исходного Dis либо вернуть EOF. */
void Vfsnative_scalartuplenext(void *frame)
{
    F_Vfsnative_scalartuplenext call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalartuplenext, ret), 3 * sizeof(WORD));
    tuple_next(call.path, STYX_TUPLE_REQUEST_TAG, call.ret, 0);
}


/* EN: Send a tuple result or a typed error on the same owner mailbox.
 * RU: Отправить кортеж или типизированную ошибку в тот же приёмник владельца. */
void Vfsnative_scalartuplereply(void *frame)
{
    F_Vfsnative_scalartuplereply call;
    if (!frame || frame == H || (uintptr_t)frame > UINTPTR_MAX - sizeof(call))
    {
        error(exBounds);
    }
    memmove(&call, frame, sizeof(call));
    unsigned char bytes[STYX_FUNCTION_FRAME_CAPACITY];
    size_t size;
    if (call.success == 1 && (call.value.t0 < INT32_MIN || call.value.t0 > INT32_MAX))
    {
        error("invalid tuple result width");
    }
    StyxTuple tuple = {.narrow = (int32_t)call.value.t0, .wide = call.value.t1};
    int encoded = call.success == 1 ? styxtuplewrite(STYX_TUPLE_REPLY_TAG, 0, &tuple, bytes, sizeof(bytes), &size) :
        styxscalarwrite(STYX_TUPLE_REPLY_TAG, STYX_SCALAR_ERROR, 0, bytes, sizeof(bytes), &size);
    if (!encoded)
    {
        error("invalid tuple reply");
    }
    dvm_scalar_owner_record(call.path, bytes, size, 0);
}



/* EN: Send one complete success record or a typed error on the owner mailbox.
 * RU: Отправить полную запись успеха или типизированную ошибку в приёмник. */
void Vfsnative_scalarrefreply(void *frame)
{
    F_Vfsnative_scalarrefreply call;
    if (!frame || frame == H || (uintptr_t)frame > UINTPTR_MAX - sizeof(call))
    {
        error(exBounds);
    }
    memmove(&call, frame, sizeof(call));
    unsigned char bytes[STYX_FUNCTION_FRAME_CAPACITY];
    size_t size;
    if (call.success == 1 && (call.value.t0 < INT32_MIN || call.value.t0 > INT32_MAX ||
        call.updated.t0 < INT32_MIN || call.updated.t0 > INT32_MAX))
    {
        error("invalid ref result width");
    }
    const StyxRefReply reply = {
        .result = {(int32_t)call.value.t0, call.value.t1},
        .updated = {(int32_t)call.updated.t0, call.updated.t1}
    };
    int encoded = call.success == 1 ? styxrefreplywrite(&reply, bytes, sizeof(bytes), &size) :
        styxscalarwrite(STYX_REF_REPLY_TAG, STYX_SCALAR_ERROR, 0, bytes, sizeof(bytes), &size);
    if (!encoded)
    {
        error("invalid ref reply");
    }
    dvm_scalar_owner_record(call.path, bytes, size, 1);
}



/* EN: Bind the named local export using the compiled argument-list signature.
 * RU: Связать именованный локальный экспорт по сигнатуре списка аргументов. */
void Vfsnative_scalarlistload(void *frame)
{
    F_Vfsnative_scalarlistload call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarlistload, ret), sizeof(*call.ret));
    dvm_scalar_load_composed(call.path, call.exportname, DVM_CALL_LIST_SIGNATURE, call.ret);
}


/* EN: Send the complete nested return or an error to the provider dispatcher.
 * RU: Отправить вложенный результат или ошибку диспетчеру провайдера. */
void Vfsnative_scalarlistreply(void *frame)
{
    F_Vfsnative_scalarlistreply call;
    if (!frame || frame == H || (uintptr_t)frame > UINTPTR_MAX - sizeof(call))
    {
        error(exBounds);
    }
    memmove(&call, frame, sizeof(call));
    unsigned char bytes[STYX_FUNCTION_FRAME_CAPACITY];
    size_t size;
    const StyxCallValues result = {3, {call.outer, call.inner, call.wide}};
    int encoded = call.success == 1 ?
        styxcallrecordwrite(STYX_CALL_LIST_REPLY_TAG, styxcall_list_result, &result, bytes, sizeof(bytes), &size) :
        styxscalarwrite(STYX_CALL_LIST_REPLY_TAG, STYX_SCALAR_ERROR, 0, bytes, sizeof(bytes), &size);
    if (!encoded)
    {
        error("invalid argument-list reply");
    }
    dvm_scalar_owner_record(call.path, bytes, size, 0);
}



/* EN: Decode a complete argument list using the verified owner profile.
 * RU: Прочитать список аргументов по проверенному профилю владельца. */
static int decode_list(unsigned long tag, const void *bytes, size_t size, void *result)
{
    return styxcallrecordread(tag, styxcall_list_args, bytes, size, result);
}


/* EN: Return readiness followed by the four validated call cells to the owner.
 * RU: Вернуть признак готовности и четыре проверенных ячейки владельцу. */
void Vfsnative_scalarlistnext(void *frame)
{
    F_Vfsnative_scalarlistnext call;
    WORD result[5] = {0};
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarlistnext, ret), sizeof(result));
    StyxCallValues arguments = {0};
    result[0] = dvm_scalar_owner_next(call.path, STYX_CALL_LIST_REQUEST_TAG, decode_list, &arguments, 0);
    for (size_t i = 0; i < arguments.count; i++)
    {
        result[i + 1] = arguments.cells[i];
    }
    dvm_sys_scalar_result(result, call.ret, sizeof(result));
}
