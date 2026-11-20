/* Copyright (c) 2026 bhgv
 * EN: Scalar interface admission over existing Unibus record files.
 * RU: Допуск скалярного интерфейса через существующие файлы Unibus.
 */
#include "scalar_interface.h"
#include "flashvm/srecord.h"
#include "record_write.h"
#include <string.h>

/* EN/RU: See scalar_interface.h; no replay follows an uncertain write.
 * См. scalar_interface.h; неоднозначная запись не повторяется. */
DVM_LaunchDelivery styxscalaroffer(DVM_Provider *peer, unsigned long tag,
    const char *name, uint32_t signature)
{
    size_t length = styxscalar_name_length(name);
    unsigned char data[INTERFACE_HEADER];
    unsigned char record[STYX_SCALAR_INTERFACE_CAPACITY];
    size_t used;
    styxscalar_interface_header(data, signature);
    if (!tag || !length || fvm_srecord_write_pair(tag, data, sizeof(data), name, length,
            record, sizeof(record), &used) != FVM_S_OK)
    {
        return DVM_LAUNCH_NOT_SENT;
    }
    return styxterminalrecordwrite(peer, record, used);
}


/* EN/RU: Refuse an incompatible interface before opening the request file.
 * Отклонить несовместимый интерфейс до открытия файла запросов. */
DVM_LaunchDelivery styxscalarcallchecked(DVM_Provider *peer, unsigned long call_tag,
    unsigned long offer_tag, const void *offer, size_t size, const char *name,
    uint32_t signature, int64_t argument)
{
    if (!styxscalaraccept(offer_tag, offer, size, name, signature))
    {
        return DVM_LAUNCH_NOT_SENT;
    }
    return styxscalarcallfile(peer, call_tag, argument);
}
