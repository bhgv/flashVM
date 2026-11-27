/* Copyright (c) 2026 bhgv
 * EN: Check admission and wire encoding of owner-supplied export catalogues.
 * RU: Проверить допуск и кодирование каталогов экспортов владельца.
 */
#include "publish_profile.h"


/* EN: Check ordered publication, reverse qualification and atomic refusal.
 * RU: Проверить порядок, встречную квалификацию и атомарный отказ. */
static int publish_profile(void)
{
    const StyxCallShape shape = {"(s)", "(s)"};
    unsigned char offers[3 * STYX_SHAPED_OFFER_MAX], profile[sizeof(offers)];
    size_t scalar, managed, used, count;
    StyxPublishedExport entries[3] = {0};
    if (!styxloadedoffer("add", DVM_SCALAR_INT_SIGNATURE, offers, sizeof(offers), &scalar) ||
        !styxshapedoffer("label", 17, &shape, offers + scalar, sizeof(offers) - scalar, &managed) ||
        !styxpublishprofile(offers, scalar + managed, NULL, 1,
            profile, sizeof(profile), &used, entries, 3, &count))
    {
        return 0;
    }
    FVM_SRecord record;
    size_t consumed;
    if (count != 2 || strcmp(entries[0].name, "add") || entries[0].scoped ||
        strcmp(entries[1].name, "label") || !entries[1].scoped ||
        entries[1].request != STYX_MANAGED_REQUEST_TAG || strcmp(entries[1].shape.arguments, "(s)") ||
        used <= scalar || memcmp(profile, offers, scalar) ||
        fvm_srecord_read(profile + scalar, used - scalar, STYX_SHAPED_OFFER_MAX, &record, &consumed) != FVM_S_OK ||
        consumed != used - scalar || record.tag != STYX_SCOPED_RECORD_OFFER_TAG)
    {
        return 0;
    }
    if (!styxpublishprofile(offers, scalar + managed, NULL, 0,
        profile, sizeof(profile), &used, entries, 3, &count) || entries[1].scoped ||
        used != scalar + managed || memcmp(profile, offers, used))
    {
        return 0;
    }
    memcpy(offers + scalar + managed, offers, scalar);
    return !styxpublishprofile(offers, scalar + managed + scalar, NULL, 1,
            profile, sizeof(profile), &used, entries, 3, &count) &&
        !styxpublishprofile(offers, scalar + managed - 1, NULL, 1,
            profile, sizeof(profile), &used, entries, 3, &count) &&
        !styxpublishprofile(offers, scalar, "other", 1,
            profile, sizeof(profile), &used, entries, 3, &count) &&
        !styxpublishprofile(offers, scalar + managed, NULL, 1,
            profile, 1, &used, entries, 3, &count) &&
        count == 2 && used == scalar + managed && !entries[1].scoped &&
        !memcmp(profile, offers, used);
}
