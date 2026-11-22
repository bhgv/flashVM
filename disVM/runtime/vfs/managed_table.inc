/* Copyright (c) 2026 bhgv
 * EN: Recreate hybrid tables locally and compute key-level copy-back patches.
 * RU: Локальное восстановление гибридных таблиц и патчи по изменившимся ключам.
 */


typedef struct TableDecode
{
    Fht *table; /* Owned temporary table, published only after validation succeeds. */
    int patch; /* Applying a patch may overwrite an existing key. */
} TableDecode;


/* EN: Apply a verified row to temporary native storage, preserving key namespaces.
 * RU: Применить проверенную строку к временному хранилищу с раздельными видами ключей. */
static int table_decode_row(void *state, const StyxTableKey *wire, int remove, int64_t value)
{
    TableDecode *decode = state;
    FhtKey key = {.kind = wire->text ? FHT_TEXT : FHT_INDEX, .index = wire->index,
        .text = (const char *)wire->bytes, .size = wire->size};
    if (remove)
    {
        FhtStatus status = fht_delete(decode->table, key);
        return status == FHT_OK || status == FHT_MISSING;
    }
    int64_t old;
    return (decode->patch || fht_get(decode->table, key, &old) == FHT_MISSING) &&
        fht_set(decode->table, key, value) == FHT_OK;
}


/* EN: Build an unpublished native table and reject duplicate snapshot keys.
 * RU: Собрать неопубликованную таблицу и отклонить дубликаты ключей снимка. */
static Fht *table_decode(const void *data, size_t size)
{
    TableDecode decode = {.table = fht_create(NULL)};
    if (!decode.table || !styxmanaged_table_visit(data, size, 0, table_decode_row, &decode))
    {
        fht_destroy(decode.table);
        return NULL;
    }
    return decode.table;
}


typedef struct TableAllocation
{
    Fht *payload; /* Own pending table until transferred into the native handle. */
    void *result; /* Own reconstructed VM reference on success. */
} TableAllocation;


/* EN: Allocate a real local Bigtable handle under the caller's error boundary.
 * RU: Выделить настоящий локальный дескриптор Bigtable в границах обработки ошибок. */
static int table_allocate(void *state)
{
    TableAllocation *allocation = state;
    allocation->result = dvm_bigtable_from_payload(allocation->payload);
    if (!allocation->result)
    {
        return 0;
    }
    allocation->payload = NULL;
    return 1;
}


/* EN: Reconstruct native storage before allocating the local GC-managed owner.
 * RU: Восстановить нативное хранилище до выделения управляемого владельца. */
static void *table_read(const void *data, size_t size)
{
    TableAllocation allocation = {.payload = table_decode(data, size)};
    if (allocation.payload)
    {
        kprotect(table_allocate, &allocation);
    }
    fht_destroy(allocation.payload);
    return allocation.result;
}


typedef struct TableWire
{
    unsigned char *rows; /* Borrowed bounded S-record payload buffer. */
    unsigned char *key; /* Borrowed scratch for the largest permitted key. */
    size_t size; /* Complete rows already written. */
    size_t capacity; /* Capacity of both buffers in bytes. */
} TableWire;


/* EN: Append one set/remove operation, leaving the length unchanged on overflow.
 * RU: Дописать одну операцию записи или удаления. При переполнении длина прежняя. */
static int table_write_row(TableWire *wire, const FhtKey *key, int remove, int64_t value)
{
    unsigned char scalar[16];
    size_t key_size, value_size, used;
    int written = key->kind == FHT_INDEX ?
        styxscalarwrite(STYX_CALL_BIG_TAG, STYX_SCALAR_VALUE, key->index, wire->key, wire->capacity, &key_size) :
        fvm_srecord_write(STYX_MANAGED_STRING_TAG, key->text, key->size,
            wire->key, wire->capacity, &key_size) == FVM_S_OK;
    if (!written || (!remove && !styxscalarwrite(STYX_CALL_BIG_TAG, STYX_SCALAR_VALUE,
        value, scalar, sizeof(scalar), &value_size)))
    {
        return 0;
    }
    size_t extent = wire->capacity - wire->size;
    unsigned char *output = wire->rows + wire->size;
    int valid = remove ?
        fvm_srecord_write(STYX_MANAGED_TABLE_REMOVE_TAG, wire->key, key_size, output, extent, &used) == FVM_S_OK :
        fvm_srecord_write_pair(STYX_MANAGED_TABLE_PAIR_TAG, wire->key, key_size, scalar, value_size,
            output, extent, &used) == FVM_S_OK;
    if (valid)
    {
        wire->size += used;
    }
    return valid;
}


/* EN: Take one ordered snapshot while the native table guard is held.
 * RU: Снять упорядоченный снимок под нативным guard таблицы. */
static int table_snapshot(const Fht *table, void *state)
{
    TableWire *wire = state;
    FhtKeys *keys = NULL;
    if (fht_count(table) > wire->capacity / 2 || fht_keys(table, &keys) != FHT_OK)
    {
        return 0;
    }
    int valid = 1;
    for (size_t i = 0; valid && i < keys->count; i++)
    {
        const FhtKey *key = &keys->items[i];
        int64_t value;
        valid = fht_get(table, *key, &value) == FHT_OK && table_write_row(wire, key, 0, value);
    }
    fht_keys_destroy(keys);
    return valid;
}


/* EN: Encode an exact native table handle as independently owned wire bytes.
 * RU: Закодировать точный нативный дескриптор в независимые сетевые байты. */
static int table_write(void *object, void *bytes, size_t capacity, size_t *used)
{
    unsigned char *scratch = malloc(capacity * 2);
    TableWire wire = {.rows = scratch, .key = scratch ? scratch + capacity : NULL, .capacity = capacity};
    int valid = scratch && dvm_bigtable_snapshot(object, table_snapshot, &wire) &&
        fvm_srecord_write(STYX_MANAGED_TABLE_TAG, wire.rows, wire.size, bytes, capacity, used) == FVM_S_OK;
    free(scratch);
    return valid;
}


/* EN: Return the boundary between ascending numeric and insertion-order text keys.
 * RU: Найти границу числовых ключей и текстовых ключей в порядке вставки. */
static size_t first_text(const FhtKeys *keys)
{
    size_t i = 0;
    while (i < keys->count && keys->items[i].kind == FHT_INDEX)
    {
        i++;
    }
    return i;
}


/* EN: Keep the longest result prefix already in the caller's old order.
 * Mark each retained old key. A moved suffix will be deleted and reinserted.
 * RU: Сохранить самый длинный префикс результата, встречающийся в прежнем
 * порядке. Отметить сохранённые ключи. Перемещённый хвост вставится заново. */
static size_t table_order(const FhtKeys *old, const FhtKeys *next, unsigned char *retained)
{
    size_t right = first_text(next);
    for (size_t left = first_text(old); left < old->count && right < next->count; left++)
    {
        const FhtKey *a = &old->items[left], *b = &next->items[right];
        if (a->size == b->size && !memcmp(a->text, b->text, a->size))
        {
            retained[left] = 1;
            right++;
        }
    }
    return right;
}


typedef struct TableDiff
{
    Fht *baseline; /* Owned original native contents, borrowed during the callback. */
    TableWire wire; /* Borrowed output and progress of the generated patch. */
} TableDiff;


/* EN: Encode deletions first, then changed/new pairs in their resulting order.
 * RU: Кодировать сперва удаления, затем изменённые и новые пары в итоговом порядке. */
static int table_diff(const Fht *current, void *state)
{
    TableDiff *diff = state;
    FhtKeys *old = NULL, *next = NULL;
    int valid = fht_count(current) <= STYX_MANAGED_LIST_LIMIT / 2 &&
        fht_keys(diff->baseline, &old) == FHT_OK && fht_keys(current, &next) == FHT_OK;
    unsigned char *retained = valid ? calloc(old->count ? old->count : 1, 1) : NULL;
    valid = retained != NULL;
    if (valid)
    {
        size_t new_tail = table_order(old, next, retained);
        for (size_t i = 0; valid && i < old->count; i++)
        {
            const FhtKey *key = &old->items[i];
            int64_t value;
            if ((key->kind == FHT_TEXT && !retained[i]) || fht_get(current, *key, &value) == FHT_MISSING)
            {
                valid = table_write_row(&diff->wire, key, 1, 0);
            }
        }
        for (size_t i = 0; valid && i < next->count; i++)
        {
            const FhtKey *key = &next->items[i];
            int64_t value, previous;
            valid = fht_get(current, *key, &value) == FHT_OK;
            if (valid && ((key->kind == FHT_TEXT && i >= new_tail) ||
                fht_get(diff->baseline, *key, &previous) != FHT_OK || previous != value))
            {
                valid = table_write_row(&diff->wire, key, 0, value);
            }
        }
    }
    free(retained);
    fht_keys_destroy(old);
    fht_keys_destroy(next);
    return valid;
}


/* EN: Compare the provider's live table against its original wire definition.
 * Return a complete key patch or zero bytes when every entry is unchanged.
 * RU: Сравнить таблицу провайдера с исходным определением из запроса.
 * Вернуть полный патч ключей или ноль байтов при отсутствии изменений. */
static int table_patch(void *object, const FVM_SRecord *original, void *bytes, size_t capacity, size_t *used)
{
    unsigned char *scratch = malloc(capacity * 2);
    TableDiff diff = {.baseline = table_decode(original->payload + 1, original->size - 1),
        .wire = {.rows = scratch, .key = scratch ? scratch + capacity : NULL, .capacity = capacity}};
    int valid = scratch && diff.baseline && dvm_bigtable_snapshot(object, table_diff, &diff);
    *used = 0;
    if (valid && diff.wire.size)
    {
        valid = fvm_srecord_write(STYX_MANAGED_TABLE_PATCH_TAG, diff.wire.rows, diff.wire.size,
            bytes, capacity, used) == FVM_S_OK;
    }
    fht_destroy(diff.baseline);
    free(scratch);
    return valid;
}


/* EN: Copy the caller's current table and apply only provider-modified keys.
 * The replacement remains private until every table and reply slot is checked.
 * RU: Скопировать актуальную таблицу клиента и применить лишь изменённые ключи.
 * Замена остаётся частной до проверки всех таблиц и слотов ответа. */
static Fht *stage_table_edit(const Fht *current, void *state)
{
    FVM_SRecord *patch = state;
    FhtKeys *keys = NULL;
    TableDecode staged = {.table = fht_create(NULL), .patch = 1};
    int valid = staged.table && fht_keys(current, &keys) == FHT_OK;
    for (size_t i = 0; valid && i < keys->count; i++)
    {
        FhtKey key = keys->items[i];
        int64_t value;
        valid = fht_get(current, key, &value) == FHT_OK && fht_set(staged.table, key, value) == FHT_OK;
    }
    fht_keys_destroy(keys);
    if (!valid || !styxmanaged_table_visit(patch->payload, patch->size, 1, table_decode_row, &staged))
    {
        fht_destroy(staged.table);
        return NULL;
    }
    return staged.table;
}
