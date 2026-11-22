/* Copyright (c) 2026 bhgv
 * EN: Verify hybrid table ordering, record boundaries and the typed graph envelope.
 * RU: Проверка порядка ключей, границ записей и типизированного графа таблиц.
 */


/* EN: Use real S-record encoders to test a valid snapshot and malformed updates.
 * RU: Собрать настоящие S-записи для проверки снимка и ошибочных обновлений. */
static int table_records(void)
{
    unsigned char key[24], value[16], row[48], list[112], bytes[144], definition[176];
    size_t key_size, value_size, row_size, used;
    if (!styxscalarwrite(STYX_CALL_BIG_TAG, STYX_SCALAR_VALUE, INT64_MAX, key, sizeof(key), &key_size) ||
        !styxscalarwrite(STYX_CALL_BIG_TAG, STYX_SCALAR_VALUE, INT64_MIN, value, sizeof(value), &value_size) ||
        fvm_srecord_write_pair(STYX_MANAGED_TABLE_PAIR_TAG, key, key_size, value, value_size,
            row, sizeof(row), &row_size) != FVM_S_OK)
    {
        return 0;
    }
    unsigned char index = 0;
    if (fvm_srecord_write(STYX_MANAGED_TABLE_TAG, row, row_size, bytes, sizeof(bytes), &used) != FVM_S_OK ||
        !styxmanaged_table_visit(bytes, used, 0, NULL, NULL) ||
        styxmanaged_table_visit(bytes, used - 1, 0, NULL, NULL) ||
        fvm_srecord_write_pair(STYX_MANAGED_REF_TAG, &index, 1, bytes, used, definition,
            sizeof(definition), &used) != FVM_S_OK)
    {
        return 0;
    }
    FVM_SRecord argument = {.tag = STYX_MANAGED_POSITION_GRAPH_TAG, .payload = definition, .size = used};
    if (!styxmanaged_request_payload("(h)", &argument) || styxmanaged_request_payload("(r[b])", &argument))
    {
        return 0;
    }
    memcpy(list, row, row_size);
    memcpy(list + row_size, row, row_size);
    if (fvm_srecord_write(STYX_MANAGED_TABLE_TAG, list, 2 * row_size, bytes, sizeof(bytes), &used) != FVM_S_OK ||
        styxmanaged_table_visit(bytes, used, 0, NULL, NULL))
    {
        return 0;
    }
    size_t remove;
    if (fvm_srecord_write(STYX_MANAGED_TABLE_REMOVE_TAG, key, key_size, list, sizeof(list), &remove) != FVM_S_OK)
    {
        return 0;
    }
    memcpy(list + remove, row, row_size);
    if (fvm_srecord_write(STYX_MANAGED_TABLE_PATCH_TAG, list, remove + row_size, bytes,
            sizeof(bytes), &used) != FVM_S_OK || !styxmanaged_table_visit(bytes, used, 1, NULL, NULL))
    {
        return 0;
    }
    memmove(list + row_size, list, remove);
    memcpy(list, row, row_size);
    return fvm_srecord_write(STYX_MANAGED_TABLE_PATCH_TAG, list, remove + row_size, bytes,
        sizeof(bytes), &used) == FVM_S_OK && !styxmanaged_table_visit(bytes, used, 1, NULL, NULL);
}
