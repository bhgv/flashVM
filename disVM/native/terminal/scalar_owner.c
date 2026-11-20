/* Copyright (c) 2026 bhgv
 * EN: Read a validated argument and publish one result on the admitted return route.
 * RU: Прочитать проверенный аргумент и выдать один результат по выданному маршруту.
 */
#include "scalar_owner.h"
#include "record_write.h"
#include "cancel.h"
#include <stdlib.h>


typedef enum OwnerState
{
    OWNER_WAITING, /* No request consumed. */
    OWNER_ADMITTED, /* One request handed off, reply permitted. */
    OWNER_RETIRED /* Invalid input or reply attempted. */
} OwnerState;

struct StyxScalarOwner
{
    DVM_VfsHandle inbox; /* Owned read cursor. */
    DVM_Provider *reply; /* Owned exact reply grant. */
    unsigned long request_tag; /* Required request tag. */
    unsigned long reply_tag; /* Reply route tag. */
    OwnerState state; /* Owner-only linear request state. */
};


/* EN/RU: See scalar_owner.h. Refuse invalid tags before acquiring either file.
 * См. scalar_owner.h. Отклонить неверные теги до получения файлов. */
StyxScalarOwner *styxscalarowner(DVM_Provider *inbox, unsigned long request_tag,
    DVM_Provider *reply, unsigned long reply_tag)
{
    if (!inbox || !reply || !request_tag || request_tag > UINT32_MAX ||
        !reply_tag || reply_tag > UINT32_MAX || dvm_cancel_requested(dvm_cancel_current()) ||
        (dvm_cancel_current() && !(dvm_provider_native_flags(inbox) & DVM_PROVIDER_CANCEL_PATH)))
    {
        return NULL;
    }
    StyxScalarOwner *owner = calloc(1, sizeof(*owner));
    if (!owner)
    {
        return NULL;
    }
    if (!dvm_provider_retain(reply))
    {
        free(owner);
        return NULL;
    }
    owner->reply = reply;
    if (dvm_provider_open(inbox, ".", "r", &owner->inbox) != FVM_OK ||
        (dvm_cancel_current() && !(owner->inbox.native_flags & DVM_VFS_CANCEL_DATA)))
    {
        styxscalarownerfree(owner);
        return NULL;
    }
    owner->request_tag = request_tag;
    owner->reply_tag = reply_tag;
    return owner;
}


/* EN/RU: Read precisely one complete request, with no retry after consuming bytes.
 * Прочитать ровно один полный запрос, без повтора после потребления байт. */
StyxScalarOutcome styxscalarownertake(StyxScalarOwner *owner, int64_t *argument)
{
    if (!owner || !argument || owner->state != OWNER_WAITING)
    {
        return STYX_GATEWAY_UNKNOWN;
    }
    if (dvm_cancel_requested(dvm_cancel_current()) ||
        (dvm_cancel_current() && !(owner->inbox.native_flags & DVM_VFS_CANCEL_DATA)))
    {
        owner->state = OWNER_RETIRED;
        return STYX_GATEWAY_UNKNOWN;
    }
    unsigned char bytes[STYX_SCALAR_RECORD_CAPACITY + 1];
    DVM_IoResult got = dvm_vfs_read_chunk(&owner->inbox, bytes, sizeof(bytes));
    if (got.stop == DVM_IO_AGAIN && !got.count && !(got.flags & DVM_IO_UNCERTAIN))
    {
        return STYX_GATEWAY_PENDING;
    }
    owner->state = OWNER_RETIRED;
    StyxScalarKind kind;
    int64_t value;
    if (got.status != FVM_OK || !(got.flags & DVM_IO_PREFIX) || (got.flags & DVM_IO_UNCERTAIN) ||
        !styxscalarread(owner->request_tag, bytes, got.count, &kind, &value) || kind != STYX_SCALAR_CALL)
    {
        return STYX_GATEWAY_UNKNOWN;
    }
    owner->state = OWNER_ADMITTED;
    *argument = value;
    return STYX_GATEWAY_VALUE;
}


/* EN/RU: Consume reply eligibility before its one possible write.
 * Поглотить разрешение ответа до единственной возможной записи. */
DVM_LaunchDelivery styxscalarownersend(StyxScalarOwner *owner, int success, int64_t result)
{
    if (!owner || owner->state != OWNER_ADMITTED)
    {
        return DVM_LAUNCH_NOT_SENT;
    }
    owner->state = OWNER_RETIRED;
    unsigned char bytes[STYX_SCALAR_RECORD_CAPACITY];
    size_t used;
    StyxScalarKind kind = success == 1 ? STYX_SCALAR_VALUE : STYX_SCALAR_ERROR;
    return styxscalarwrite(owner->reply_tag, kind, result, bytes, sizeof(bytes), &used) ?
        styxterminalrecordwrite(owner->reply, bytes, used) : DVM_LAUNCH_NOT_SENT;
}


/* EN/RU: Retire both native file owners once no serialized operation is active.
 * Освободить оба файла после завершения последовательных операций. */
void styxscalarownerfree(StyxScalarOwner *owner)
{
    if (owner)
    {
        dvm_vfs_close(&owner->inbox);
        dvm_provider_release(owner->reply);
        free(owner);
    }
}
