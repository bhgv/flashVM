/* Copyright (c) 2026 bhgv
 * EN: Preserve scalar values and delivery uncertainty across independent Styx routes.
 * RU: Сохранение скалярных значений и неопределённости доставки на раздельных маршрутах Styx.
 */
#include "scalar_route.h"
#include "record_write.h"
#include "cancel.h"
#include "flashvm/srecord.h"


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


/* EN/RU: Write one orderly end marker using the existing request tag.
 * Записать признак завершения с существующим тегом запросов. */
DVM_LaunchDelivery styxscalarclosefile(DVM_Provider *peer, unsigned long tag)
{
    const unsigned char end = 5; /* Distinct from scalar and profile records. */
    unsigned char bytes[STYX_SCALAR_RECORD_CAPACITY];
    size_t used;
    return tag && fvm_srecord_write(tag, &end, sizeof(end), bytes, sizeof(bytes), &used) == FVM_S_OK ?
        styxterminalrecordwrite(peer, bytes, used) : DVM_LAUNCH_NOT_SENT;
}


/* EN/RU: Accept only the exact tagged end marker; no scalar argument is read.
 * Принять точный признак завершения с тегом, без скалярного аргумента. */
int styxscalarclosed(unsigned long tag, const void *bytes, size_t size)
{
    FVM_SRecord record;
    size_t consumed;
    return tag && fvm_srecord_read(bytes, size, STYX_SCALAR_RECORD_CAPACITY, &record, &consumed) == FVM_S_OK &&
        consumed == size && record.tag == tag && record.size == 1 && record.payload[0] == 5;
}
