/* Copyright (c) 2026 bhgv
 * EN: Check the interface before opening the route and never replay submissions.
 * RU: Сверять интерфейс перед открытием маршрута и не повторять запросы.
 */
#include "scalar_gateway.h"
#include "cancel.h"
#include <stdlib.h>


struct StyxScalarGateway
{
    DVM_Provider *request; /* Owned exact file grant for one request. */
    DVM_VfsHandle reply; /* Owned cursor for the reverse S-record. */
    unsigned long call_tag; /* Nonzero unsigned 32-bit route tag. */
    unsigned long reply_tag; /* Nonzero unsigned 32-bit reply tag. */
    int attempted; /* Sticky even on transport error. */
    int awaiting; /* Reply can be read after an acknowledged request. */
};


/* EN/RU: Check the exact profile before acquiring either call file.
 * Проверить точный профиль до получения файлов вызова. */
static int matches(DVM_Provider *profile, unsigned long tag, const char *name, uint32_t signature)
{
    DVM_VfsHandle cursor = {0};
    if (!profile || (dvm_cancel_current() &&
        !(dvm_provider_native_flags(profile) & DVM_PROVIDER_CANCEL_PATH)) ||
        dvm_provider_open(profile, ".", "r", &cursor) != FVM_OK)
    {
        return 0;
    }
    int matched = 0;
    if (!dvm_cancel_current() || (cursor.native_flags & DVM_VFS_CANCEL_DATA))
    {
        unsigned char frame[STYX_SCALAR_INTERFACE_CAPACITY + 1];
        DVM_IoResult got = dvm_vfs_read_chunk(&cursor, frame, sizeof(frame));
        matched = got.status == FVM_OK && (got.flags & DVM_IO_PREFIX) &&
            !(got.flags & DVM_IO_UNCERTAIN) && styxscalaraccept(tag, frame, got.count, name, signature);
    }
    dvm_vfs_close(&cursor);
    return matched;
}


/* EN/RU: See scalar_gateway.h; no route is installed after a failed check.
 * См. scalar_gateway.h; после отказа маршрут не создаётся. */
StyxScalarGateway *styxscalargateway(DVM_Provider *profile, unsigned long profile_tag,
    const char *name, uint32_t signature, DVM_Provider *request, unsigned long call_tag,
    DVM_Provider *reply, unsigned long reply_tag)
{
    if (!request || !reply || !call_tag || call_tag > UINT32_MAX ||
        !reply_tag || reply_tag > UINT32_MAX || dvm_cancel_requested(dvm_cancel_current()) ||
        (dvm_cancel_current() && !(dvm_provider_native_flags(reply) & DVM_PROVIDER_CANCEL_PATH)) ||
        !matches(profile, profile_tag, name, signature))
    {
        return NULL;
    }
    StyxScalarGateway *gateway = calloc(1, sizeof(*gateway));
    if (!gateway)
    {
        return NULL;
    }
    if (!dvm_provider_retain(request))
    {
        free(gateway);
        return NULL;
    }
    gateway->request = request;
    if (dvm_provider_open(reply, ".", "r", &gateway->reply) != FVM_OK ||
        (dvm_cancel_current() && !(gateway->reply.native_flags & DVM_VFS_CANCEL_DATA)))
    {
        styxscalargatewayfree(gateway);
        return NULL;
    }
    gateway->call_tag = call_tag;
    gateway->reply_tag = reply_tag;
    return gateway;
}


/* EN/RU: Attempt the request at most once for this gateway.
 * Попытаться отправить запрос не более одного раза для этого шлюза. */
DVM_LaunchDelivery styxscalargatewaysend(StyxScalarGateway *gateway, int64_t argument)
{
    if (!gateway || gateway->attempted)
    {
        return DVM_LAUNCH_NOT_SENT;
    }
    gateway->attempted = 1;
    DVM_LaunchDelivery delivery = styxscalarcallfile(gateway->request, gateway->call_tag, argument);
    gateway->awaiting = delivery == DVM_LAUNCH_DELIVERED;
    return delivery;
}


/* EN/RU: Consume a complete scalar reply, allowing only empty pending reads to repeat.
 * Принять полный скалярный ответ. Повторять лишь пустое чтение в ожидании. */
StyxScalarOutcome styxscalargatewayreceive(StyxScalarGateway *gateway, int64_t *value)
{
    if (!gateway || !gateway->awaiting || !value)
    {
        return STYX_GATEWAY_UNKNOWN;
    }
    if (dvm_cancel_requested(dvm_cancel_current()) ||
        (dvm_cancel_current() && !(gateway->reply.native_flags & DVM_VFS_CANCEL_DATA)))
    {
        gateway->awaiting = 0;
        return STYX_GATEWAY_UNKNOWN;
    }
    unsigned char frame[STYX_SCALAR_RECORD_CAPACITY + 1];
    DVM_IoResult got = dvm_vfs_read_chunk(&gateway->reply, frame, sizeof(frame));
    if (got.stop == DVM_IO_AGAIN && !got.count && !(got.flags & DVM_IO_UNCERTAIN))
    {
        return STYX_GATEWAY_PENDING;
    }
    gateway->awaiting = 0;
    StyxScalarKind kind;
    int64_t result;
    if (got.status != FVM_OK || !(got.flags & DVM_IO_PREFIX) || (got.flags & DVM_IO_UNCERTAIN) ||
        !styxscalarread(gateway->reply_tag, frame, got.count, &kind, &result) || kind == STYX_SCALAR_CALL)
    {
        return STYX_GATEWAY_UNKNOWN;
    }
    *value = result;
    return kind == STYX_SCALAR_VALUE ? STYX_GATEWAY_VALUE : STYX_GATEWAY_ERROR;
}


/* EN/RU: Free this gateway after all serialized users leave.
 * Освободить шлюз после завершения всех последовательных операций. */
void styxscalargatewayfree(StyxScalarGateway *gateway)
{
    if (gateway)
    {
        dvm_vfs_close(&gateway->reply);
        dvm_provider_release(gateway->request);
        free(gateway);
    }
}
