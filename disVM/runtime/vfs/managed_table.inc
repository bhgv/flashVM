/* Copyright (c) 2026 bhgv
 * EN: Recreate hybrid tables locally and compute key-level copy-back patches.
 * RU: Локальное восстановление гибридных таблиц и патчи по изменившимся ключам.
 */


#include "../../native/terminal/managed_table_state.inc"


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
    TableAllocation allocation = {.payload = wire_table_decode(data, size)};
    if (allocation.payload)
    {
        kprotect(table_allocate, &allocation);
    }
    fht_destroy(allocation.payload);
    return allocation.result;
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


/* EN: Compare the provider's live table against its original wire definition.
 * Return a complete key patch or zero bytes when every entry is unchanged.
 * RU: Сравнить таблицу провайдера с исходным определением из запроса.
 * Вернуть полный патч ключей или ноль байтов при отсутствии изменений. */
static int table_patch(void *object, const FVM_SRecord *original, void *bytes, size_t capacity, size_t *used)
{
    unsigned char *scratch = malloc(capacity * 2);
    TableDiff diff = {.baseline = wire_table_decode(original->payload + 1, original->size - 1),
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


/* EN: Stage the checked patch against current native storage under its owner guard.
 * RU: Подготовить патч из текущего содержимого под блокировкой владельца. */
static Fht *stage_table_edit(const Fht *current, void *state)
{
    const FVM_SRecord *patch = state;
    return wire_table_stage(current, patch->payload, patch->size);
}
