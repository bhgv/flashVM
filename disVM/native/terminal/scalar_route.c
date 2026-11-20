/* Copyright (c) 2026 bhgv
 * EN: Preserve scalar values and delivery uncertainty across independent Styx routes.
 * RU: Сохранение скалярных значений и неопределённости доставки на раздельных маршрутах Styx.
 */
#include "scalar_route.h"
#include "record_write.h"
#include "cancel.h"


/* EN/RU: See scalar_route.h; encode the method's single signed argument.
 * См. scalar_route.h; закодировать один знаковый аргумент метода. */
DVM_LaunchDelivery styxscalarcallfile(DVM_Provider *peer, unsigned long tag, int64_t argument)
{
    unsigned char bytes[STYX_SCALAR_RECORD_CAPACITY];
    size_t used;
    return styxscalarwrite(tag, STYX_SCALAR_CALL, argument, bytes, sizeof(bytes), &used) ?
        styxterminalrecordwrite(peer, bytes, used) : DVM_LAUNCH_NOT_SENT;
}


/* EN/RU: See scalar_route.h; never replay after the callback has run.
 * См. scalar_route.h; не повторять после исполнения callback. */
DVM_LaunchDelivery styxscalarreplyfile(DVM_Provider *peer, unsigned long call_tag,
    const void *request, size_t size, unsigned long reply_tag,
    int (*method)(void *, int64_t, int64_t *), void *state)
{
    unsigned char bytes[STYX_SCALAR_RECORD_CAPACITY];
    size_t used;
    if (!peer || dvm_cancel_requested(dvm_cancel_current()) ||
        (dvm_cancel_current() && !(dvm_provider_native_flags(peer) & DVM_PROVIDER_CANCEL_PATH)) ||
        !styxscalarrespond(call_tag, request, size, reply_tag, method, state, bytes, sizeof(bytes), &used))
    {
        return DVM_LAUNCH_NOT_SENT;
    }
    return styxterminalrecordwrite(peer, bytes, used);
}
