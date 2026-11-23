/* Copyright (c) 2026 bhgv
 * EN: Validate admitted UniBus channel grants, guards and reply envelopes.
 * RU: Проверка полномочий, веток и ответов каналов UniBus.
 */
#include "styxrpc/channel.h"
#include "call_record.h"
#include "scalar_record.h"
#include <string.h>

#define CHANNEL_SEND 25UL        /* Capability key followed by one element. */
#define CHANNEL_RECEIVE 26UL     /* Capability key with no element. */
#define CHANNEL_SCALAR 27UL      /* Legacy int-only reply. */
#define CHANNEL_TYPED_REPLY 30UL /* Selected guard byte and optional element. */


/* EN: Find an unambiguous registered key with one checked element type.
 * RU: Найти однозначный зарегистрированный ключ с проверенным типом элемента. */
static const char *grant_shape(const StyxRpcChannelGrant *grants, size_t count, uint32_t key)
{
    if (!grants || !key || !count || count > STYXRPC_CHANNEL_SLOTS)
    {
        return NULL;
    }
    const char *shape = NULL;
    for (size_t i = 0; i < count; ++i)
    {
        if (grants[i].key == key)
        {
            if (shape)
            {
                return NULL;
            }
            shape = grants[i].shape;
        }
    }
    if (!shape || !memchr(shape, 0, STYXRPC_MANAGED_SHAPE) || shape[0] != '(' ||
        strpbrk(shape, "ch"))
    {
        return NULL;
    }
    const char *end = styxshape_end(shape + 1);
    return end && !strcmp(end, ")") ? shape : NULL;
}


/* EN: Parse a signed int element and reject invalid tags, widths or kinds.
 * RU: Разобрать int, исключив неверные теги, разрядность и вид записи. */
static int integer_value(const void *value, size_t size, int64_t *number)
{
    StyxScalarKind kind;
    return styxscalarread(STYX_CALL_INT_TAG, value, size, &kind, number) &&
        kind == STYX_SCALAR_VALUE && *number >= INT32_MIN && *number <= INT32_MAX;
}


/* EN: Check the element's entire S-record, including nested typed values.
 * RU: Проверить целую S-запись элемента, включая вложенные типизированные данные. */
static int element_valid(const char *shape, int typed, const void *value, size_t size)
{
    if (!value || !size || size > STYXRPC_MANAGED_LIMIT)
    {
        return 0;
    }
    if (!typed)
    {
        int64_t number;
        return integer_value(value, size, &number);
    }
    FVM_SRecord record;
    size_t used;
    return fvm_srecord_read(value, size, STYXRPC_MANAGED_LIMIT, &record, &used) == FVM_S_OK &&
        used == size && styxmanaged_channel_value(shape, &record, NULL, NULL, 0);
}


/* EN: Validate one request and report whether all guards are int-only.
 * RU: Проверить операцию и сообщить, все ли ветки работают с int. */
static int check(const StyxRpcChannelGrant *grants, size_t grant_count,
    const StyxRpcSelection *operation, int *typed)
{
    *typed = 0;
    if (!operation || !operation->count || operation->count > STYXRPC_CHANNEL_SLOTS ||
        (operation->mode == STYXRPC_CHANNEL_ONE ? operation->count != 1 :
         operation->mode != STYXRPC_CHANNEL_ALT && operation->mode != STYXRPC_CHANNEL_TRY &&
         operation->mode != STYXRPC_CHANNEL_WAIT))
    {
        return 0;
    }
    for (size_t i = 0; i < operation->count; ++i)
    {
        const StyxRpcGuard *guard = &operation->guards[i];
        const char *shape = grant_shape(grants, grant_count, guard->key);
        if (!shape)
        {
            return 0;
        }
        const int managed = strcmp(shape, "(i)") != 0;
        if (guard->receive == 1 ? guard->size != 0 :
            guard->receive != 0 || !element_valid(shape, managed, guard->value, guard->size))
        {
            return 0;
        }
        *typed |= managed;
    }
    return 1;
}


/* EN: Write the canonical key and optional encoded element as one guard.
 * RU: Записать канонический ключ и необязательный элемент одной веткой. */
static int write_guard(const StyxRpcGuard *guard, void *bytes, size_t capacity, size_t *used)
{
    const uint32_t key = guard->key;
    unsigned char identity[] = {key >> 24, key >> 16, key >> 8, key};
    return fvm_srecord_write_pair(guard->receive ? CHANNEL_RECEIVE : CHANNEL_SEND, identity, sizeof(identity),
        guard->value, guard->size, bytes, capacity, used) == FVM_S_OK;
}


/* EN/RU: See channel.h. / См. channel.h. */
StyxRpcStatus styxrpc_channel_request(const StyxRpcChannelGrant *grants, size_t grant_count,
    const StyxRpcSelection *operation, void *scratch, size_t scratch_size,
    void *bytes, size_t capacity, size_t *used)
{
    int typed;
    if (!used)
    {
        return STYXRPC_INVALID;
    }
    *used = 0;
    if (!bytes || !check(grants, grant_count, operation, &typed))
    {
        return STYXRPC_INVALID;
    }
    if (capacity > STYXRPC_MANAGED_LIMIT)
    {
        capacity = STYXRPC_MANAGED_LIMIT;
    }
    if (operation->mode == STYXRPC_CHANNEL_ONE)
    {
        return write_guard(operation->guards, bytes, capacity, used) ? STYXRPC_OK : STYXRPC_INVALID;
    }
    if (!scratch)
    {
        return STYXRPC_INVALID;
    }
    size_t size = 0;
    for (size_t i = 0; i < operation->count; ++i)
    {
        size_t length;
        if (!write_guard(operation->guards + i, (unsigned char *)scratch + size, scratch_size - size, &length))
        {
            return STYXRPC_INVALID;
        }
        size += length;
    }
    return fvm_srecord_write(operation->mode, scratch, size, bytes, capacity, used) == FVM_S_OK ?
        STYXRPC_OK : STYXRPC_INVALID;
}


/* EN: Decode one guard into a temporary selection without publishing it.
 * RU: Разобрать ветку во временную структуру без её публикации. */
static int read_guard(const FVM_SRecord *record, StyxRpcSelection *operation)
{
    if (operation->count == STYXRPC_CHANNEL_SLOTS || record->size < 4 ||
        (record->tag != CHANNEL_SEND && record->tag != CHANNEL_RECEIVE))
    {
        return 0;
    }
    const unsigned char *id = record->payload;
    const uint32_t key = (uint32_t)id[0] << 24 | (uint32_t)id[1] << 16 | (uint32_t)id[2] << 8 | id[3];
    operation->guards[operation->count++] =
        (StyxRpcGuard){key, record->tag == CHANNEL_RECEIVE, id + 4, record->size - 4};
    return 1;
}


/* EN/RU: See channel.h. / См. channel.h. */
StyxRpcStatus styxrpc_channel_read(const StyxRpcChannelGrant *grants, size_t grant_count,
    const void *bytes, size_t size, StyxRpcSelection *operation)
{
    FVM_SRecord record;
    size_t used;
    if (!operation || !bytes || size > STYXRPC_MANAGED_LIMIT ||
        fvm_srecord_read(bytes, size, STYXRPC_MANAGED_LIMIT, &record, &used) != FVM_S_OK || used != size)
    {
        return STYXRPC_INVALID;
    }
    StyxRpcSelection decoded = {0};
    if (record.tag == STYXRPC_CHANNEL_ALT || record.tag == STYXRPC_CHANNEL_TRY || record.tag == STYXRPC_CHANNEL_WAIT)
    {
        decoded.mode = (StyxRpcChannelMode)record.tag;
        const unsigned char *child = record.payload;
        for (size_t remaining = record.size; remaining; remaining -= used, child += used)
        {
            if (fvm_srecord_read(child, remaining, STYXRPC_MANAGED_LIMIT, &record, &used) != FVM_S_OK ||
                !read_guard(&record, &decoded))
            {
                return STYXRPC_INVALID;
            }
        }
    }
    else if (!read_guard(&record, &decoded))
    {
        return STYXRPC_INVALID;
    }
    int typed;
    if (!check(grants, grant_count, &decoded, &typed))
    {
        return STYXRPC_INVALID;
    }
    *operation = decoded;
    return STYXRPC_OK;
}


/* EN: Check that an ordinal can occur for this selection mode.
 * RU: Проверить, допустим ли номер ветки в этом режиме выбора. */
static int ordinal_allowed(const StyxRpcSelection *operation, unsigned ordinal)
{
    return ordinal ? ordinal <= operation->count :
        operation->mode == STYXRPC_CHANNEL_TRY || operation->mode == STYXRPC_CHANNEL_WAIT;
}


/* EN/RU: See channel.h. / См. channel.h. */
StyxRpcStatus styxrpc_channel_reply(const StyxRpcChannelGrant *grants, size_t grant_count,
    const StyxRpcSelection *operation, unsigned selected, const void *value, size_t size,
    void *bytes, size_t capacity, size_t *used)
{
    int typed;
    if (!used)
    {
        return STYXRPC_INVALID;
    }
    *used = 0;
    if (!bytes || !check(grants, grant_count, operation, &typed) ||
        (selected != 255 && !ordinal_allowed(operation, selected)))
    {
        return STYXRPC_INVALID;
    }
    int receive = selected && selected != 255 && operation->guards[selected - 1].receive;
    if (receive ? !element_valid(grant_shape(grants, grant_count, operation->guards[selected - 1].key), typed,
        value, size) : size != 0)
    {
        return STYXRPC_INVALID;
    }
    if (typed)
    {
        unsigned char index = (unsigned char)selected;
        if (capacity > STYXRPC_MANAGED_LIMIT)
        {
            capacity = STYXRPC_MANAGED_LIMIT;
        }
        return fvm_srecord_write_pair(CHANNEL_TYPED_REPLY, &index, 1, value, size, bytes, capacity, used) == FVM_S_OK ?
            STYXRPC_OK : STYXRPC_INVALID;
    }
    int64_t integer = 0;
    if (receive && !integer_value(value, size, &integer))
    {
        return STYXRPC_INVALID;
    }
    if (selected != 255 && operation->mode != STYXRPC_CHANNEL_ONE)
    {
        integer = ((int64_t)selected << 32) | (uint32_t)integer;
    }
    return styxscalarwrite(CHANNEL_SCALAR, selected == 255 ? STYX_SCALAR_ERROR : STYX_SCALAR_VALUE,
        integer, bytes, capacity, used) ? STYXRPC_OK : STYXRPC_INVALID;
}


/* EN: Decode an int-only outcome, including an ALT selector in the upper word.
 * RU: Разобрать скалярный ответ int, включая номер ALT в старшем слове. */
static StyxRpcStatus scalar_result(const StyxRpcSelection *operation, const void *bytes, size_t size,
    StyxRpcChannelResult *result)
{
    StyxScalarKind kind;
    int64_t value;
    if (!styxscalarread(CHANNEL_SCALAR, bytes, size, &kind, &value))
    {
        return STYXRPC_INVALID;
    }
    if (kind == STYX_SCALAR_ERROR)
    {
        return STYXRPC_REMOTE_ERROR;
    }
    uint32_t bits = (uint32_t)value;
    unsigned index = operation->mode == STYXRPC_CHANNEL_ONE ? 1 : (unsigned)((uint64_t)value >> 32);
    if (kind != STYX_SCALAR_VALUE || !ordinal_allowed(operation, index) || (!index && value) ||
        (operation->mode == STYXRPC_CHANNEL_ONE && (value < INT32_MIN || value > INT32_MAX)))
    {
        return STYXRPC_INVALID;
    }
    result->selected = index;
    if (index && operation->guards[index - 1].receive)
    {
        result->integer = bits <= INT32_MAX ? (int32_t)bits : -1 - (int32_t)(UINT32_MAX - bits);
    }
    return STYXRPC_OK;
}


/* EN: Decode and validate the selected typed element against the admitted original guard.
 * RU: Разобрать и проверить типизированный элемент по допущенной исходной ветке. */
static StyxRpcStatus typed_result(const StyxRpcChannelGrant *grants, size_t grant_count,
    const StyxRpcSelection *operation, const void *bytes, size_t size, StyxRpcChannelResult *result)
{
    FVM_SRecord record;
    size_t used;
    if (!bytes || size > STYXRPC_MANAGED_LIMIT ||
        fvm_srecord_read(bytes, size, STYXRPC_MANAGED_LIMIT, &record, &used) != FVM_S_OK ||
        used != size || record.tag != CHANNEL_TYPED_REPLY || !record.size)
    {
        return STYXRPC_INVALID;
    }
    unsigned index = record.payload[0];
    if (index == 255)
    {
        return record.size == 1 ? STYXRPC_REMOTE_ERROR : STYXRPC_INVALID;
    }
    if (!ordinal_allowed(operation, index))
    {
        return STYXRPC_INVALID;
    }
    result->selected = index;
    if (!index || !operation->guards[index - 1].receive)
    {
        return record.size == 1 ? STYXRPC_OK : STYXRPC_INVALID;
    }
    const char *shape = grant_shape(grants, grant_count, operation->guards[index - 1].key);
    if (!element_valid(shape, 1, record.payload + 1, record.size - 1))
    {
        return STYXRPC_INVALID;
    }
    result->value = record.payload + 1;
    result->size = record.size - 1;
    return STYXRPC_OK;
}


/* EN/RU: See channel.h. / См. channel.h. */
StyxRpcStatus styxrpc_channel_result(const StyxRpcChannelGrant *grants, size_t grant_count,
    const StyxRpcSelection *operation, const void *bytes, size_t size, StyxRpcChannelResult *result)
{
    int typed;
    if (!result || !check(grants, grant_count, operation, &typed))
    {
        return STYXRPC_INVALID;
    }
    StyxRpcChannelResult decoded = {0};
    StyxRpcStatus status = typed ? typed_result(grants, grant_count, operation, bytes, size, &decoded) :
        scalar_result(operation, bytes, size, &decoded);
    if (status == STYXRPC_OK)
    {
        *result = decoded;
    }
    return status;
}
