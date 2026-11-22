/* Copyright (c) 2026 bhgv
 * EN: Validate portable hybrid table snapshots and ordered key patches.
 * RU: Проверка переносимых снимков гибридной таблицы и патчей ключей.
 */


/* EN: Decode an exact numeric or counted UTF-8 key from a row.
 * RU: Декодировать точный числовой ключ либо UTF-8 с явной длиной. */
static int table_key(const FVM_SRecord *row, StyxTableKey *key, size_t *used)
{
    FVM_SRecord node;
    if (fvm_srecord_read(row->payload, row->size, STYX_MANAGED_LIST_LIMIT, &node, used) != FVM_S_OK)
    {
        return 0;
    }
    if (node.tag == STYX_MANAGED_STRING_TAG)
    {
        *key = (StyxTableKey){.bytes = node.payload, .size = node.size, .text = 1};
        return fvm_utf8_validate((const char *)node.payload, node.size, NULL) == FVM_UTF8_OK;
    }
    StyxScalarKind kind;
    *key = (StyxTableKey){0};
    return node.tag == STYX_CALL_BIG_TAG && styxscalarread(node.tag, row->payload, *used, &kind, &key->index) &&
        kind == STYX_SCALAR_VALUE && key->index >= 0;
}


/* EN: Check and optionally visit rows. Snapshots use ascending numeric keys
 * followed by text in insertion order. Patches put removals before writes.
 * RU: Проверить и при необходимости обойти строки. В снимке числовые ключи
 * возрастают, текстовые следуют в порядке вставки. В патче удаления идут первыми. */
static int table_rows(const FVM_SRecord *list, int patch,
    int (*visit)(void *, const StyxTableKey *, int, int64_t), void *state)
{
    int strings = 0, writes = 0;
    int64_t previous = -1;
    for (size_t offset = 0; offset < list->size; )
    {
        FVM_SRecord row;
        StyxTableKey key;
        size_t used, first;
        if (fvm_srecord_read(list->payload + offset, list->size - offset, STYX_MANAGED_LIST_LIMIT, &row, &used) != FVM_S_OK ||
            !table_key(&row, &key, &first))
        {
            return 0;
        }
        int remove = patch && row.tag == STYX_MANAGED_TABLE_REMOVE_TAG;
        int64_t value = 0;
        if (remove)
        {
            if (writes || first != row.size)
            {
                return 0;
            }
        }
        else
        {
            StyxScalarKind kind;
            if (row.tag != STYX_MANAGED_TABLE_PAIR_TAG ||
                !styxscalarread(STYX_CALL_BIG_TAG, row.payload + first, row.size - first, &kind, &value) ||
                kind != STYX_SCALAR_VALUE)
            {
                return 0;
            }
            writes = 1;
        }
        if (!patch && !key.text)
        {
            if (strings || key.index <= previous)
            {
                return 0;
            }
            previous = key.index;
        }
        strings |= key.text;
        if (visit && !visit(state, &key, remove, value))
        {
            return 0;
        }
        offset += used;
    }
    return 1;
}


/* EN/RU: See call_record.h. / См. call_record.h. */
int styxmanaged_table_visit(const void *bytes, size_t size, int patch,
    int (*visit)(void *, const StyxTableKey *, int, int64_t), void *state)
{
    FVM_SRecord list;
    size_t used;
    return size && fvm_srecord_read(bytes, size, STYX_MANAGED_LIST_LIMIT, &list, &used) == FVM_S_OK &&
        used == size && list.tag == (patch ? STYX_MANAGED_TABLE_PATCH_TAG : STYX_MANAGED_TABLE_TAG) &&
        (!patch || list.size) && table_rows(&list, patch, visit, state);
}
