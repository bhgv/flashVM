/* Copyright (c) 2026 bhgv
 * EN: Wire tests for direct argument positions and nested reference indices.
 * RU: Тест протокола для прямых позиций аргументов и индексов вложенных ссылок.
 */


/* EN: Verify that presence of a scalar and a later alias cannot shift copy-back.
 * RU: Проверить, что скаляр и последующий алиас не сдвигают обратную запись. */
static int argument_position_updates(void)
{
    StyxCallShape shape = {.arguments = "(ir[s]r[s]a[r[s]])", .result = "()"};
    unsigned char values[128], text[24], object[32], payload[64], reply[16], updates[72];
    size_t argc, entry, field, extent, length;
    unsigned char zero = 0, one = 1;
    StyxReplyField fields[2] = {{reply, 0}, {updates, 0}};
    if (!styxscalarwrite(STYX_CALL_INT_TAG, STYX_SCALAR_VALUE, 9, values, sizeof(values), &argc) ||
        fvm_srecord_write(STYX_MANAGED_STRING_TAG, "seed", 4, text, sizeof(text), &field) != FVM_S_OK ||
        fvm_srecord_write_pair(STYX_MANAGED_REF_TAG, &zero, 1, text, field,
            values + argc, sizeof(values) - argc, &extent) != FVM_S_OK)
    {
        return 0;
    }
    argc += extent;
    if (fvm_srecord_write(STYX_MANAGED_REF_TAG, &zero, 1,
            values + argc, sizeof(values) - argc, &extent) != FVM_S_OK ||
        fvm_srecord_write_pair(STYX_MANAGED_REF_TAG, &one, 1, text, field,
            object, sizeof(object), &entry) != FVM_S_OK)
    {
        return 0;
    }
    argc += extent;
    if (fvm_srecord_write(STYX_MANAGED_ARRAY_TAG, object, entry, values + argc,
            sizeof(values) - argc, &extent) != FVM_S_OK ||
        fvm_srecord_write(STYX_MANAGED_NIL_TAG, NULL, 0, reply, sizeof(reply), &fields[0].size) != FVM_S_OK)
    {
        return 0;
    }
    FVM_SRecord arguments = {.tag = STYX_MANAGED_POSITION_GRAPH_TAG, .payload = values, .size = argc + extent};
    const struct
    {
        unsigned long tag; /* Selector type. */
        unsigned char selector; /* Original argument position or object index. */
        int valid; /* Expected validation result. */
    } cases[] = {
        {STYX_MANAGED_ARG_UPDATE_TAG, 1, 1},
        {STYX_MANAGED_ARG_UPDATE_TAG, 0, 0},
        {STYX_MANAGED_ARG_UPDATE_TAG, 2, 0},
        {STYX_MANAGED_ARG_UPDATE_TAG, 3, 0},
        {STYX_MANAGED_REF_TAG, 0, 0},
        {STYX_MANAGED_REF_TAG, 1, 1},
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
    {
        unsigned char header[] = {cases[i].selector, 1};
        if (fvm_srecord_write_pair(cases[i].tag, header, sizeof(header), text, field,
                payload, sizeof(payload), &length) != FVM_S_OK ||
            fvm_srecord_write(STYX_MANAGED_POSITION_UPDATES_TAG, payload, length, updates,
                sizeof(updates), &fields[1].size) != FVM_S_OK ||
            styxmanaged_graph_reply(&shape, &arguments, fields) != cases[i].valid)
        {
            return 0;
        }
    }
    return 1;
}


/* EN: Preserve ordinary field delta decoding while checking the argument position.
 * RU: Сохранить декодирование дельт полей с проверкой позиции аргумента. */
static int position_field_deltas(void)
{
    const char *shape = "(ir[s])";
    unsigned char values[64], text[24], payload[48], updates[64], expanded[64];
    size_t length, field, entry, used;
    unsigned char index = 0;
    if (!styxscalarwrite(STYX_CALL_INT_TAG, STYX_SCALAR_VALUE, 9, values, sizeof(values), &length) ||
        fvm_srecord_write(STYX_MANAGED_STRING_TAG, "seed", 4, text, sizeof(text), &field) != FVM_S_OK ||
        fvm_srecord_write_pair(STYX_MANAGED_REF_TAG, &index, 1, text, field,
            values + length, sizeof(values) - length, &entry) != FVM_S_OK)
    {
        return 0;
    }
    FVM_SRecord arguments = {.tag = STYX_MANAGED_POSITION_FLAT_TAG, .payload = values, .size = length + entry};
    StyxFieldDelta changes[STYX_CALL_VALUES];
    size_t count;
    payload[0] = 1;
    for (unsigned char position = 0; position <= 1; position++)
    {
        unsigned char header[] = {position, 1};
        if (fvm_srecord_write_pair(STYX_MANAGED_ARG_UPDATE_TAG, header, sizeof(header), text, field,
                payload + 1, sizeof(payload) - 1, &entry) != FVM_S_OK ||
            fvm_srecord_write(STYX_MANAGED_DELTA_TAG, payload, 1 + entry, updates, sizeof(updates), &used) != FVM_S_OK ||
            styxmanaged_deltas_base(shape, updates, used, changes, &count, &arguments,
                expanded, sizeof(expanded)) != position)
        {
            return 0;
        }
    }
    return count == 1 && changes[0].object == 0 && changes[0].mask == 1 &&
        !strcmp(changes[0].schema, "(s)");
}
