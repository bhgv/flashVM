/* Copyright (c) 2026 bhgv
 * EN: Scalar interface admission over existing Unibus record files.
 * RU: Допуск скалярного интерфейса через существующие файлы Unibus.
 */
#include "scalar_verify.h"
#include "flashvm/srecord.h"
#include <string.h>

/* EN/RU: Measure the borrowed nonempty name within the wire limit.
 * Измерить непустое заимствованное имя в пределах формата. */
size_t styxscalar_name_length(const char *name)
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
void styxscalar_interface_header(unsigned char *data, uint32_t signature)
{
    data[0] = INTERFACE_KIND;
    for (size_t i = INTERFACE_HEADER - 1; i > 0; i--)
    {
        data[i] = (unsigned char)signature;
        signature >>= 8;
    }
}


/* EN/RU: See scalar_interface.h; admit exactly one canonical S-record.
 * См. scalar_interface.h; допустить ровно одну каноническую S-запись. */
int styxscalaraccept(unsigned long tag, const void *bytes, size_t size,
    const char *name, uint32_t signature)
{
    size_t length = styxscalar_name_length(name);
    unsigned char data[INTERFACE_HEADER];
    FVM_SRecord record;
    size_t used;
    styxscalar_interface_header(data, signature);
    return tag && length &&
        fvm_srecord_read(bytes, size, INTERFACE_HEADER + INTERFACE_NAME_LIMIT + FVM_SRECORD_INTEGER_MAX,
            &record, &used) == FVM_S_OK &&
        size == used && record.tag == tag && record.size == sizeof(data) + length &&
        !memcmp(record.payload, data, sizeof(data)) &&
        !memcmp(record.payload + sizeof(data), name, length);
}
