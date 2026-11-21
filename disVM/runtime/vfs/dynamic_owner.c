/* Copyright (c) 2026 bhgv
 * EN: Copy scalar call words between the owner VM and its local Styx dispatcher.
 * RU: Обмен скалярными ячейками между VM владельца и локальным диспетчером Styx.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "namespace.h"
#include "runqueue.h"
#include "export.h"
#include "program_view.h"
#include "sys_capture.h"
#include "scalar_owner_internal.h"
#include "vfsnative_decl.h"
#include "raise.h"
#include "../../native/terminal/call_record.h"
#include "../../native/terminal/call_scope.h"
#include "../../native/terminal/load_profile.h"
#include "../../native/terminal/scalar_record.h"
#include "../../native/terminal/tuple_record.h"
#include <string.h>


/* EN: Clone the borrowed owner's namespace and overlay only the pinned caller.
 * Return a private owned namespace or NULL before any Dis call runs.
 * RU: Копировать namespace владельца и наложить только закреплённого клиента.
 * Вернуть свою копию либо NULL до выполнения Dis. */
DVM_Namespace *dvm_scalar_caller_namespace(DVM_Namespace *group, DVM_Provider *context)
{
    DVM_View *view = dvm_view_create(NULL, NULL, NULL);
    if (!view || !dvm_view_map(view, "/.self", context, ".", FVM_ACCESS_MASK, DVM_VIEW_TREE))
    {
        dvm_view_discard(view);
        return NULL;
    }
    DVM_Provider *projection = dvm_view_freeze(view);
    DVM_Namespace *copy = projection ? dvm_namespace_copy(group) : NULL;
    int installed = copy && dvm_program_mount_private(copy, projection, "/.prog", NULL);
    dvm_provider_release(projection);
    if (!installed)
    {
        dvm_namespace_release(copy);
        return NULL;
    }
    return copy;
}


typedef struct RawRequest
{
    StyxCallValues values; /* Four staged words consumed by the local ABI. */
    int scoped;            /* The record requires an independently captured provider. */
} RawRequest;


/* EN: Read the dispatcher's complete four-word local argument record.
 * RU: Прочитать полную локальную запись из четырёх ячеек аргументов. */
static int decode_words(unsigned long tag, const void *bytes, size_t size, void *result)
{
    RawRequest *request = result;
    request->scoped = styxcallrecordread(STYX_SCOPED_CALL_TAG, "(bbbb)", bytes, size, &request->values);
    return request->scoped || styxcallrecordread(tag, "(bbbb)", bytes, size, &request->values);
}


/* EN: Return EOF or the four staged input cells to the owner process.
 * RU: Вернуть EOF либо четыре скопированные входные ячейки процессу владельца. */
void Vfsnative_scalarrawnext(void *frame)
{
    F_Vfsnative_scalarrawnext call;
    WORD result[STYX_DYNAMIC_CELLS + 1];
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarrawnext, ret), sizeof(result));
    RawRequest decoded = {0};
    result[0] = dvm_scalar_owner_next(call.path, STYX_DYNAMIC_REQUEST_TAG, decode_words, &decoded, 1);
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)currun();
    if (result[0] == 1 && decoded.scoped && !owner->remote_call_group)
    {
        result[0] = -1;
    }
    for (size_t i = 0; i < STYX_DYNAMIC_CELLS; i++)
    {
        result[i + 1] = decoded.values.cells[i];
    }
    dvm_sys_scalar_result(result, call.ret, sizeof(result));
}


/* EN: Return only the validated prefix from the owner's scalar staging area.
 * RU: Вернуть только проверенный префикс из скалярного буфера владельца. */
void Vfsnative_scalarrawreply(void *frame)
{
    F_Vfsnative_scalarrawreply call;
    if (!frame || frame == H || (uintptr_t)frame > UINTPTR_MAX - sizeof(call))
    {
        error(exBounds);
    }
    memcpy(&call, frame, sizeof(call));
    if (call.count < 0 || call.count > STYX_DYNAMIC_CELLS)
    {
        error("invalid remote scalar result extent");
    }
    unsigned char bytes[STYX_FUNCTION_FRAME_CAPACITY];
    size_t used;
    int encoded;
    if (call.success == 1)
    {
        const LONG fields[] = {call.value.t0, call.value.t1, call.value.t2, call.value.t3};
        StyxCallValues staged = {.count = STYX_DYNAMIC_CELLS};
        for (size_t i = 0; i < (size_t)call.count; i++)
        {
            staged.cells[i] = fields[i];
        }
        encoded = styxcallrecordwrite(STYX_DYNAMIC_REPLY_TAG, "(bbbb)", &staged, bytes, sizeof(bytes), &used);
    }
    else
    {
        encoded = styxscalarwrite(STYX_DYNAMIC_REPLY_TAG, STYX_SCALAR_ERROR, 0, bytes, sizeof(bytes), &used);
    }
    if (!encoded)
    {
        error("invalid remote scalar reply");
    }
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)currun();
    DVM_Namespace *scope = owner->remote_call_group;
    owner->remote_call_group = NULL;
    dvm_namespace_release(scope);
    dvm_scalar_owner_record(call.path, bytes, used);
}
