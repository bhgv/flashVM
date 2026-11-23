/* Copyright (c) 2026 bhgv
 * EN: Exercise typed channel admission within nested managed message records.
 * RU: Проверка допуска типизированных каналов во вложенных записях сообщения.
 */


/* EN: Expected capabilities in the fixture, plus the caller-owned visit count.
 * RU: Ожидаемые полномочия в примере и счётчик обхода во владении вызывающего. */
typedef struct ChannelAdmissions
{
    uint32_t integer_key; /* Wire key of the fixture's chan of int. */
    uint32_t string_key; /* Wire key of the fixture's chan of string. */
    size_t *visited; /* Borrowed count of all admission callbacks. */
} ChannelAdmissions;


/* EN: Check exactly one schema node and its borrowed fixture key. Count visits.
 * RU: Сверить ровно один узел схемы и ключ теста. Учитывать каждый вызов. */
static int admit_channel_record(const void *context, const char *type, uint32_t key)
{
    const ChannelAdmissions *admissions = context;
    const char *end = styxshape_end(type);
    ++*admissions->visited;
    return end && end - type == 4 &&
        ((key == admissions->integer_key && !memcmp(type, "c[i]", 4)) ||
         (key == admissions->string_key && !memcmp(type, "c[s]", 4)));
}


/* EN: Encode a channel key, or nil when key is zero, in caller-owned bytes.
 * RU: Записать ключ канала либо nil для нуля в буфер вызывающего. */
static int write_channel_record(uint32_t key, unsigned char *bytes, size_t capacity, size_t *used)
{
    unsigned char identity[4] = {key >> 24, key >> 16, key >> 8, key};
    return (key ? fvm_srecord_write(STYX_MANAGED_CHANNEL_TAG, identity, sizeof(identity), bytes, capacity, used) :
        fvm_srecord_write(STYX_MANAGED_NIL_TAG, NULL, 0, bytes, capacity, used)) == FVM_S_OK;
}


/* EN: Encode one fixture containing separate keys and a nil tuple-array entry.
 * RU: Закодировать пример с разными ключами и nil внутри массива кортежей. */
static int channel_message(uint32_t first_key, uint32_t nested_key,
    unsigned char *bytes, size_t capacity, size_t *used)
{
    unsigned char tuple[64], entry[64], array[128], outer[192];
    size_t label, channel, first, second, direct, nested, payload;
    return fvm_srecord_write(STYX_MANAGED_STRING_TAG, "key", 3, tuple, sizeof(tuple), &label) == FVM_S_OK &&
        write_channel_record(nested_key, tuple + label, sizeof(tuple) - label, &channel) &&
        fvm_srecord_write(STYX_CALL_TUPLE_TAG, tuple, label + channel, array, sizeof(array), &first) == FVM_S_OK &&
        write_channel_record(0, tuple + label, sizeof(tuple) - label, &channel) &&
        fvm_srecord_write(STYX_CALL_TUPLE_TAG, tuple, label + channel, array + first, sizeof(array) - first,
            &second) == FVM_S_OK &&
        write_channel_record(first_key, entry, sizeof(entry), &direct) &&
        fvm_srecord_write(STYX_MANAGED_ARRAY_TAG, array, first + second, outer, sizeof(outer),
            &nested) == FVM_S_OK &&
        fvm_srecord_write_pair(STYX_CALL_TUPLE_TAG, entry, direct, outer, nested,
            array, sizeof(array), &payload) == FVM_S_OK &&
        fvm_srecord_write(STYX_MANAGED_GRAPH_TAG, array, payload, bytes, capacity, used) == FVM_S_OK;
}


/* EN: Require reverse grants and issued unbuffered keys to stay inside the proper message direction.
 * RU: Проверить, что встречные выдачи и ключи без буфера допускаются в нужном направлении. */
static int reverse_channel_records(const ChannelAdmissions *admissions)
{
    unsigned char bytes[32];
    unsigned char grant[8] = {128, 0, 0, 1, 128, 0, 0, 7};
    unsigned char key[4] = {1, 35, 69, 103};
    FVM_SRecord frame = {.tag = STYX_MANAGED_GRAPH_TAG, .payload = bytes};
    if (fvm_srecord_write(STYX_MANAGED_CHANNEL_GRANT_TAG, grant, sizeof(grant),
            bytes, sizeof(bytes), &frame.size) != FVM_S_OK ||
        styxmanaged_request_payload("(c[i])", &frame) ||
        styxmanaged_channel_payload("(c[i])", &frame, admit_channel_record, admissions) ||
        !styxmanaged_channel_value("(c[i])", &frame, admit_channel_record, admissions, 1))
    {
        return 0;
    }
    grant[0] = 0;
    if (fvm_srecord_write(STYX_MANAGED_CHANNEL_GRANT_TAG, grant, sizeof(grant),
            bytes, sizeof(bytes), &frame.size) != FVM_S_OK ||
        styxmanaged_channel_value("(c[i])", &frame, admit_channel_record, admissions, 1))
    {
        return 0;
    }
    unsigned char module[STYX_CHANNEL_EXPORT_TOKEN_SIZE + 4] = {0};
    module[0] = 0x51;
    module[sizeof(module) - 1] = 7;
    if (fvm_srecord_write(STYX_MANAGED_CHANNEL_MODULE_GRANT_TAG, module, sizeof(module),
            bytes, sizeof(bytes), &frame.size) != FVM_S_OK ||
        styxmanaged_request_payload("(c[i])", &frame) ||
        styxmanaged_channel_payload("(c[i])", &frame, admit_channel_record, admissions) ||
        !styxmanaged_channel_value("(c[i])", &frame, admit_channel_record, admissions, 1))
    {
        return 0;
    }
    module[sizeof(module) - 1] = 0;
    if (fvm_srecord_write(STYX_MANAGED_CHANNEL_MODULE_GRANT_TAG, module, sizeof(module),
            bytes, sizeof(bytes), &frame.size) != FVM_S_OK ||
        styxmanaged_channel_value("(c[i])", &frame, NULL, NULL, 1))
    {
        return 0;
    }
    return fvm_srecord_write(STYX_MANAGED_UNBUFFERED_CHANNEL_TAG, key, sizeof(key),
            bytes, sizeof(bytes), &frame.size) == FVM_S_OK &&
        !styxmanaged_channel_value("(c[i])", &frame, admit_channel_record, admissions, 1) &&
        styxmanaged_channel_value("(c[i])", &frame, NULL, NULL, 1);
}


/* EN: Check exact nested type/identity matching. Nil never reaches the visitor.
 * RU: Сверить точные вложенные типы и ключи. Обходчик для nil не вызывается. */
static int channel_admission_records(void)
{
    const char *shape = "((c[i]a[(sc[s])]))";
    size_t visited = 0;
    const ChannelAdmissions admissions = {0x01234567, 0x12345678, &visited};
    const uint32_t keys[] = {admissions.string_key, 0x13572468, admissions.integer_key};
    unsigned char bytes[192];
    for (size_t i = 0; i < sizeof(keys) / sizeof(*keys); i++)
    {
        size_t length, used;
        FVM_SRecord decoded;
        visited = 0;
        if (!channel_message(admissions.integer_key, keys[i], bytes, sizeof(bytes), &length) ||
            fvm_srecord_read(bytes, length, sizeof(bytes), &decoded, &used) != FVM_S_OK || used != length ||
            !styxmanaged_request_payload(shape, &decoded) ||
            styxmanaged_channel_payload(shape, &decoded, admit_channel_record, &admissions) != (i == 0) ||
            visited != 2)
        {
            return 0;
        }
        decoded.tag = STYX_MANAGED_REQUEST_TAG;
        if (styxmanaged_channel_payload(shape, &decoded, admit_channel_record, &admissions) || visited != 2)
        {
            return 0;
        }
    }
    return reverse_channel_records(&admissions);
}
