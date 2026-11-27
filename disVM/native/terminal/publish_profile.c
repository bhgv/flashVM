/* Copyright (c) 2026 bhgv
 * EN: Validate a complete Dis export catalogue and stage its public Styx records.
 * RU: Проверить каталог экспортов Dis целиком и подготовить открытые записи Styx.
 */
#include "publish_profile.h"
#include "call_scope.h"
#include "channel_export.h"
#include "flashvm/srecord.h"
#include <string.h>


/* EN: Reject names that cannot be exact children of the mounted module.
 * RU: Отклонить имена, которые нельзя опубликовать как дочерние узлы модуля. */
static int export_name(const char *name)
{
    return *name && !strchr(name, '/') && !strchr(name, '\t') &&
        strcmp(name, ".") && strcmp(name, "..") && strcmp(name, ".interface") &&
        strcmp(name, STYX_MANAGED_DELTA_FILE) && strcmp(name, STYX_CHANNEL_EXPORT_FILE) &&
        strcmp(name, STYX_CHANNEL_PROFILE_FILE) && strcmp(name, STYX_CHANNEL_STATUS_FILE);
}


/* EN: Read one private record and stage its descriptor and public tag.
 * RU: Прочитать частную запись и подготовить её описание и открытый тег. */
static unsigned long admit(const FVM_SRecord *record, StyxPublishedExport *entry, int scoped)
{
    uint32_t signature = 0;
    if (record->tag == STYX_SHAPED_OFFER_TAG)
    {
        entry->request = styxshapedpayload(record->payload, record->size,
            entry->name, &signature, &entry->shape);
        entry->scoped = scoped && entry->request && !styxloadedrequest(signature);
    }
    else if (record->tag == STYX_FUNCTION_PROFILE_TAG && record->size > INTERFACE_HEADER &&
        record->size <= INTERFACE_HEADER + INTERFACE_NAME_LIMIT && record->payload[0] == INTERFACE_KIND)
    {
        const size_t length = record->size - INTERFACE_HEADER;
        if (memchr(record->payload + INTERFACE_HEADER, 0, length))
        {
            return 0;
        }
        memcpy(entry->name, record->payload + INTERFACE_HEADER, length);
        for (size_t i = 1; i < INTERFACE_HEADER; ++i)
        {
            signature = (signature << 8) | record->payload[i];
        }
        entry->request = styxloadedrequest(signature);
    }
    if (!entry->request || !export_name(entry->name))
    {
        return 0;
    }
    if (entry->scoped)
    {
        return entry->request == STYX_MANAGED_REQUEST_TAG ?
            STYX_SCOPED_RECORD_OFFER_TAG : STYX_SCOPED_PATH_OFFER_TAG;
    }
    return record->tag;
}


/* EN/RU: See publish_profile.h. / См. publish_profile.h. */
int styxpublishprofile(const void *offers, size_t size, const char *name, int scoped,
    void *profile, size_t capacity, size_t *used, StyxPublishedExport *exports, size_t limit, size_t *count)
{
    unsigned char staged[STYX_MOUNTED_EXPORT_LIMIT * STYX_SHAPED_OFFER_MAX];
    StyxPublishedExport entries[STYX_MOUNTED_EXPORT_LIMIT] = {0};
    if (!offers || !size || size > sizeof(staged) || !profile || !used || !exports ||
        !limit || limit > STYX_MOUNTED_EXPORT_LIMIT || !count)
    {
        return 0;
    }
    const unsigned char *cursor = offers;
    size_t length = 0;
    size_t parsed = 0;
    while (size)
    {
        FVM_SRecord record;
        size_t consumed, encoded;
        if (parsed == limit || fvm_srecord_read(cursor, size, STYX_SHAPED_OFFER_MAX,
            &record, &consumed) != FVM_S_OK)
        {
            return 0;
        }
        StyxPublishedExport *entry = &entries[parsed];
        unsigned long tag = admit(&record, entry, scoped);
        if (!tag || (name && (consumed != size || strcmp(name, entry->name))))
        {
            return 0;
        }
        for (size_t i = 0; i < parsed; ++i)
        {
            if (!strcmp(entries[i].name, entry->name))
            {
                return 0;
            }
        }
        if (fvm_srecord_write(tag, record.payload, record.size, staged + length,
            sizeof(staged) - length, &encoded) != FVM_S_OK)
        {
            return 0;
        }
        length += encoded;
        cursor += consumed;
        size -= consumed;
        ++parsed;
    }
    if (length > capacity)
    {
        return 0;
    }
    memcpy(profile, staged, length);
    memcpy(exports, entries, parsed * sizeof(*exports));
    *used = length;
    *count = parsed;
    return 1;
}
