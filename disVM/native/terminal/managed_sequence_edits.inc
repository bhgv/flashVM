/* Copyright (c) 2026 bhgv
 * EN: Bounded resynchronization and compact replacements in original coordinates.
 * RU: Ограниченный поиск совпадений и компактные замены в исходных координатах.
 */

#define STYX_EDIT_ANCHOR 16U /* Minimum matching byte sequence for resynchronization. */
#define STYX_EDIT_WINDOW 64U /* Maximum shift probed at a mismatch. */


/* EN: Find a nearby matched region after an insertion, deletion or replacement.
 * RU: Найти ближайший общий участок после вставки, удаления либо замены. */
static void find_edit(const unsigned char *old, size_t left, const unsigned char *next, size_t right,
    size_t *removed, size_t *inserted)
{
    *removed = left;
    *inserted = right;
    if (left < STYX_EDIT_ANCHOR || right < STYX_EDIT_ANCHOR)
    {
        return;
    }
    for (size_t shift = 1; shift <= STYX_EDIT_WINDOW; shift++)
    {
        int fits_left = shift <= left - STYX_EDIT_ANCHOR;
        int fits_right = shift <= right - STYX_EDIT_ANCHOR;
        if (fits_right && !memcmp(old, next + shift, STYX_EDIT_ANCHOR))
        {
            *removed = 0;
            *inserted = shift;
            return;
        }
        if (fits_left && !memcmp(old + shift, next, STYX_EDIT_ANCHOR))
        {
            *removed = shift;
            *inserted = 0;
            return;
        }
        if (fits_left && fits_right && !memcmp(old + shift, next + shift, STYX_EDIT_ANCHOR))
        {
            *removed = *inserted = shift;
            return;
        }
    }
}


/* EN: Write a compact series of replacements when at least two edits pay off.
 * RU: Записать компактную серию замен, если выгодны хотя бы две правки. */
static int managed_edits_write(const FVM_SRecord *before, const FVM_SRecord *after,
    void *bytes, size_t capacity, size_t *used)
{
    if (!patchable(before->tag) || before->tag != after->tag ||
        before->size > STYX_MANAGED_LIST_LIMIT || after->size > STYX_MANAGED_LIST_LIMIT)
    {
        return 0;
    }
    size_t left = before->size, right = after->size, a = 0, b = 0;
    while (left && right && before->payload[left - 1] == after->payload[right - 1])
    {
        left--;
        right--;
    }
    unsigned char payload[STYX_MANAGED_LIST_LIMIT];
    size_t size = 0, edits = 0;
    while (a < left || b < right)
    {
        if (a < left && b < right && before->payload[a] == after->payload[b])
        {
            a++;
            b++;
            continue;
        }
        size_t removed, inserted;
        find_edit(before->payload + a, left - a, after->payload + b, right - b, &removed, &inserted);
        if (a > UINT16_MAX || removed > UINT16_MAX || inserted > UINT16_MAX ||
            size + 6 + inserted + 4 >= after->size)
        {
            return 0;
        }
        payload[size++] = a >> 8;
        payload[size++] = a;
        payload[size++] = removed >> 8;
        payload[size++] = removed;
        payload[size++] = inserted >> 8;
        payload[size++] = inserted;
        memcpy(payload + size, after->payload + b, inserted);
        size += inserted;
        a += removed;
        b += inserted;
        edits++;
    }
    return edits > 1 && fvm_srecord_write(STYX_MANAGED_EDIT_TAG, payload, size, bytes, capacity, used) == FVM_S_OK;
}


/* EN: Validate every edit and reconstruct unchanged spans from the original.
 * RU: Проверить каждую правку и восстановить неизменённые участки из исходного значения. */
static int expand_edits(const FVM_SRecord *before, const FVM_SRecord *patch,
    unsigned char *scratch, size_t capacity, void *bytes, size_t limit, size_t *used)
{
    size_t input = 0, source = 0, size = 0, edits = 0;
    while (input < patch->size)
    {
        if (patch->size - input < 6)
        {
            return 0;
        }
        const unsigned char *head = patch->payload + input;
        size_t at = (size_t)head[0] << 8 | head[1];
        size_t remove = (size_t)head[2] << 8 | head[3];
        size_t insert = (size_t)head[4] << 8 | head[5];
        input += 6;
        if ((!remove && !insert) || at < source || (edits && at == source) ||
            at > before->size || remove > before->size - at || insert > patch->size - input ||
            at - source > capacity - size || insert > capacity - size - (at - source))
        {
            return 0;
        }
        memcpy(scratch + size, before->payload + source, at - source);
        size += at - source;
        memcpy(scratch + size, patch->payload + input, insert);
        size += insert;
        source = at + remove;
        input += insert;
        edits++;
    }
    size_t tail = before->size - source;
    if (edits < 2 || tail > capacity - size || size + tail > STYX_MANAGED_LIST_LIMIT)
    {
        return 0;
    }
    memcpy(scratch + size, before->payload + source, tail);
    return fvm_srecord_write(before->tag, scratch, size + tail, bytes, limit, used) == FVM_S_OK;
}
