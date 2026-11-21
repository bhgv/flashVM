/* Copyright (c) 2026 bhgv
 * EN: Generic native address-policy ownership, independent of service queues and VM linkage.
 * RU: Общий native address-policy ownership, независимо от service queues и VM linkage.
 */
#include "address_internal.h"
#include "../cancel.h"
#include <stdlib.h>


/* EN/RU: See route.h; no callbacks before successful wrapper publication, failed allocation still consumes inner.
 * См. route.h; callbacks не вызываются до success, failed allocation также поглощает inner.
 */
DVM_LaunchAddress *dvm_launch_address_guard(DVM_LaunchAddress *inner, DVM_LaunchCheck check,
    void *context, void (*release)(void *))
{
    if (!inner || !check)
    {
        dvm_launch_address_free(inner);
        return NULL;
    }
    DVM_LaunchAddress *address = calloc(1, sizeof(*address));
    if (!address)
    {
        dvm_launch_address_free(inner);
        return NULL;
    }
    address->inner = inner;
    address->check = check;
    address->context = context;
    address->release = release;
    return address;
}


/* EN/RU: See route.h; endpoint release neither closes its service nor cancels already acknowledged requests.
 * См. route.h; release endpoint не закрывает service и не отменяет acknowledged requests.
 */
void dvm_launch_address_free(DVM_LaunchAddress *address)
{
    while (address)
    {
        DVM_LaunchAddress *inner = address->inner; /* No recursion even for deeply nested native policy wrappers. */
        if (address->release)
        {
            address->release(address->context);
        }
        if (address->dispose)
        {
            address->dispose(address);
        }
        free(address);
        address = inner;
    }
}


// EN/RU: See route.h; leaf owns only its native transport context.
// См. route.h; адрес владеет только native transport context.
DVM_LaunchAddress *dvm_launch_address_transport(DVM_LaunchSend send, void *state, void (*release)(void *))
{
    DVM_LaunchAddress *address = send ? calloc(1, sizeof(*address)) : NULL;
    if (address)
    {
        address->send = send;
        address->context = state;
        address->release = release;
    }
    return address;
}


/* EN/RU: Construct an opt-in caller-scoped leaf, leaving failed ownership with the caller.
 * Создать адрес с контекстом вызывающего, при отказе сохранив владение. */
DVM_LaunchAddress *dvm_launch_address_process_transport(DVM_LaunchSendProcess send, void *state, void (*release)(void *))
{
    DVM_LaunchAddress *address = send ? calloc(1, sizeof(*address)) : NULL;
    if (address)
    {
        address->process_send = send;
        address->context = state;
        address->release = release;
    }
    return address;
}


// EN/RU: See route.h; never retry a possibly admitted request.
// См. route.h; не повторять запрос с неопределённым результатом.
DVM_LaunchDelivery dvm_launch_address_send_process(DVM_LaunchAddress *address, uint32_t pid,
    const void *arguments, size_t size)
{
    if (!address || size > DVM_LAUNCH_ARGUMENT_LIMIT || (size && !arguments))
    {
        return DVM_LAUNCH_NOT_SENT;
    }
    while (address->inner)
    {
        if (dvm_cancel_requested(dvm_cancel_current()) || address->check(address->context) != 1)
        {
            return DVM_LAUNCH_NOT_SENT;
        }
        address = address->inner;
    }
    if (address->process_send)
    {
        return pid && !dvm_cancel_requested(dvm_cancel_current()) ?
            address->process_send(address->context, pid, arguments, size) : DVM_LAUNCH_NOT_SENT;
    }
    return address->send && !dvm_cancel_requested(dvm_cancel_current()) ?
        address->send(address->context, arguments, size) : DVM_LAUNCH_NOT_SENT;
}


/* EN/RU: Preserve existing callers that send without a process selection.
 * Сохранить прежнюю отправку без выбора процесса. */
DVM_LaunchDelivery dvm_launch_address_send(DVM_LaunchAddress *address, const void *arguments, size_t size)
{
    return dvm_launch_address_send_process(address, 0, arguments, size);
}



/* EN/RU: See route.h; inspect the immutable leaf only.
 * См. route.h; проверить только неизменяемый конечный адрес. */
int dvm_launch_address_uses_process(const DVM_LaunchAddress *address)
{
    while (address && address->inner)
    {
        address = address->inner;
    }
    return address && address->process_send;
}
