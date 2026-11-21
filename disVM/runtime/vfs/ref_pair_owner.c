/* Copyright (c) 2026 bhgv
 * EN: Bridge a two-ref call through the shared Dis owner service.
 * RU: Передача вызова с двумя ref через общий сервис владельца Dis.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "namespace.h"
#include "raise.h"
#include "sys_capture.h"
#include "scalar_owner_internal.h"
#include "vfsnative_decl.h"
#include "../../native/terminal/ref_pair_record.h"
#include "../../native/terminal/scalar_record.h"
#include "../../native/terminal/scalar_verify.h"
#include <string.h>


/* EN: Link the original named export with the exact compiled two-ref signature.
 * RU: Связать именованный исходный экспорт по точной сигнатуре двух ref. */
void Vfsnative_scalarrefpairload(void *frame)
{
    F_Vfsnative_scalarrefpairload call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarrefpairload, ret), sizeof(*call.ret));
    dvm_scalar_load_composed(call.path, call.exportname, DVM_REF_PAIR_SIGNATURE, call.ret);
}


/* EN: Adapt the mailbox decoder to the typed local aggregate.
 * RU: Применить декодер приёмника к типизированному локальному значению. */
static int decode_pair(unsigned long tag, const void *bytes, size_t size, void *result)
{
    return styxrefpairread(tag, bytes, size, result);
}


/* EN: Return the unique object count followed by their snapshots. EOF is zero.
 * RU: Вернуть число уникальных объектов и их снимки. Ноль означает EOF. */
void Vfsnative_scalarrefpairnext(void *frame)
{
    F_Vfsnative_scalarrefpairnext call;
    StyxRefPair pair = {0};
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Vfsnative_scalarrefpairnext, ret), 5 * sizeof(WORD));
    int ready = dvm_scalar_owner_next(call.path, STYX_REF_PAIR_REQUEST_TAG, decode_pair, &pair, 1);
    const WORD cells[] = {ready == 1 ? (WORD)pair.count : ready, pair.objects[0].narrow, pair.objects[0].wide,
        pair.objects[1].narrow, pair.objects[1].wide};
    dvm_sys_scalar_result(cells, call.ret, sizeof(cells));
}


/* EN: Convert checked NC64 cells to the wire tuple, rejecting wide int values.
 * RU: Преобразовать ячейки NC64 в кортеж, отклонив int вне диапазона. */
static StyxTuple reply_cell(WORD narrow, LONG wide)
{
    if (narrow < INT32_MIN || narrow > INT32_MAX)
    {
        error("invalid ref pair result width");
    }
    return (StyxTuple){(int32_t)narrow, wide};
}


/* EN: Publish one complete reply or an error on the private owner mailbox.
 * RU: Отправить полный ответ или ошибку в служебный приёмник владельца. */
void Vfsnative_scalarrefpairreply(void *frame)
{
    F_Vfsnative_scalarrefpairreply call;
    if (!frame || frame == H || (uintptr_t)frame > UINTPTR_MAX - sizeof(call))
    {
        error(exBounds);
    }
    memmove(&call, frame, sizeof(call));
    unsigned char bytes[STYX_FUNCTION_FRAME_CAPACITY];
    size_t size;
    int encoded;
    if (call.success == 1)
    {
        if (call.count < 1 || call.count > 2)
        {
            error("invalid ref pair owner count");
        }
        StyxRefPair pair = {.count = (size_t)call.count, .result = reply_cell(call.value.t0, call.value.t1)};
        pair.objects[0] = reply_cell(call.first.t0, call.first.t1);
        if (call.count == 2)
        {
            pair.objects[1] = reply_cell(call.second.t0, call.second.t1);
        }
        encoded = styxrefpairwrite(STYX_REF_PAIR_REPLY_TAG, &pair, bytes, sizeof(bytes), &size);
    }
    else
    {
        encoded = styxscalarwrite(STYX_REF_PAIR_REPLY_TAG, STYX_SCALAR_ERROR, 0, bytes, sizeof(bytes), &size);
    }
    if (!encoded)
    {
        error("invalid ref pair owner reply");
    }
    dvm_scalar_owner_record(call.path, bytes, size, 1);
}
