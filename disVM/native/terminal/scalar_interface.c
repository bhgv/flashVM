/* Copyright (c) 2026 bhgv
 * EN: Scalar interface admission over existing Unibus record files.
 * RU: Допуск скалярного интерфейса через существующие файлы Unibus.
 */
#include "scalar_interface.h"
#include "flashvm/srecord.h"
#include "record_write.h"
#include <string.h>

#define INTERFACE_KIND 4U /* Distinct from scalar call/value/error. */
#define INTERFACE_HEADER 5U /* Kind and exact 32-bit link signature. */
#define INTERFACE_NAME_LIMIT 48U /* Bytes excluding terminator. */


/* EN/RU: Measure the borrowed nonempty name within the wire limit.
 * Измерить непустое заимствованное имя в пределах формата. */
static size_t name_length(const char *name)
{
    size_t size = 0;
    if (name)
    {
        while (size <= INTERFACE_NAME_LIMIT && name[size])
        {
            size++;
        }
    }
    return size <= INTERFACE_NAME_LIMIT ? size : 0;
}


/* EN/RU: Encode a portable signature without host frame or pointer data.
 * Закодировать сигнатуру без данных нативного кадра и указателей. */
static void header(unsigned char *data, uint32_t signature)
{
    data[0] = INTERFACE_KIND;
    for (size_t i = INTERFACE_HEADER - 1; i > 0; i--)
    {
        data[i] = (unsigned char)signature;
        signature >>= 8;
    }
}


/* EN/RU: See scalar_interface.h; no replay follows an uncertain write.
 * См. scalar_interface.h; неоднозначная запись не повторяется. */
DVM_LaunchDelivery styxscalaroffer(DVM_Provider *peer, unsigned long tag,
    const char *name, uint32_t signature)
{
    size_t length = name_length(name);
    unsigned char data[INTERFACE_HEADER];
    unsigned char record[STYX_SCALAR_INTERFACE_CAPACITY];
    size_t used;
    header(data, signature);
    if (!tag || !length || fvm_srecord_write_pair(tag, data, sizeof(data), name, length,
            record, sizeof(record), &used) != FVM_S_OK)
    {
        return DVM_LAUNCH_NOT_SENT;
    }
    return styxterminalrecordwrite(peer, record, used);
}


/* EN/RU: See scalar_interface.h; admit exactly one canonical S-record.
 * См. scalar_interface.h; допустить ровно одну каноническую S-запись. */
int styxscalaraccept(unsigned long tag, const void *bytes, size_t size,
    const char *name, uint32_t signature)
{
    size_t length = name_length(name);
    unsigned char data[INTERFACE_HEADER];
    FVM_SRecord record;
    size_t used;
    header(data, signature);
    return tag && length &&
        fvm_srecord_read(bytes, size, INTERFACE_HEADER + INTERFACE_NAME_LIMIT + FVM_SRECORD_INTEGER_MAX,
            &record, &used) == FVM_S_OK &&
        size == used && record.tag == tag && record.size == sizeof(data) + length &&
        !memcmp(record.payload, data, sizeof(data)) &&
        !memcmp(record.payload + sizeof(data), name, length);
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
