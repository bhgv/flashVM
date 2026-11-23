/* Copyright (c) 2026 bhgv
 * EN: Use the existing Dis selector codec with a separate opened Styx fid state.
 * RU: Использование существующего селектора Dis с отдельным состоянием fid Styx.
 */
#include "collection_internal.h"
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>

typedef struct CollectionFid
{
    const StyxRpcRegistration *entry; /* Live registration through close. */
    uint64_t generation;              /* Exact current selector generation. */
    int completed;                    /* A mutation was already acknowledged. */
    int32_t popped;                   /* Immutable per-fid pop result. */
} CollectionFid;


/* EN: Acquire the selector or bind to its exact live generation.
 * RU: Получить селектор или закрепить его точное активное поколение. */
static int open_collection(void *context, unsigned access, void **cookie)
{
    const StyxRpcRegistration *node = context;
    (void)access;
    CollectionFid *fid = calloc(1, sizeof(*fid));
    if (!fid)
    {
        return 0;
    }
    fid->entry = node;
    if (node->collection_op == CollectionIndex || node->collection_op == CollectionValue)
    {
        fid->generation = styxintarrayopen(node->collection, node->collection_owner,
            node->collection_op == CollectionIndex);
        if (!fid->generation)
        {
            free(fid);
            return 0;
        }
    }
    *cookie = fid;
    return 1;
}


/* EN: Retire an index fid and all aliases to its generation.
 * RU: Закрыть fid индекса и отозвать все ссылки на его поколение. */
static void close_collection(void *cookie)
{
    CollectionFid *fid = cookie;
    if (fid->entry->collection_op == CollectionIndex)
    {
        styxintarrayclose(fid->entry->collection, fid->generation);
    }
    free(fid);
}


/* EN: Return one cached removal result from this fid, or remove the last value once.
 * RU: Вернуть сохранённый результат либо однократно извлечь последний элемент. */
static int pop_value(void *cookie, int32_t *value)
{
    CollectionFid *fid = cookie;
    StyxIntArray *array = fid->entry->collection;
    if (!fid->completed)
    {
        if (array->client || !array->length || array->pop(array->state, &fid->popped) != 1)
        {
            return 0;
        }
        array->length--;
        fid->completed = 1;
    }
    *value = fid->popped;
    return 1;
}


/* EN: Return a counted part of immutable or freshly generated collection metadata.
 * RU: Вернуть часть неизменяемых или только что сформированных метаданных коллекции. */
static int text(const char *source, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    size_t length = strlen(source);
    size_t remaining = offset < length ? length - (size_t)offset : 0;
    *size = capacity < remaining ? capacity : remaining;
    if (*size)
    {
        memcpy(bytes, source + (size_t)offset, *size);
    }
    return 1;
}


/* EN: Read the selected value, property, or per-fid pop result.
 * RU: Прочитать выбранное значение, свойство или результат pop текущего fid. */
static int read_collection(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    CollectionFid *fid = cookie;
    StyxIntArray *array = fid->entry->collection;
    switch (fid->entry->collection_op)
    {
    case CollectionLength:
    {
        char length[sizeof("4294967295\n")];
        snprintf(length, sizeof(length), "%" PRIu32 "\n", array->length);
        return text(length, offset, bytes, capacity, size);
    }
    case CollectionType:
        return text(array->contains ? "table of int\n" : "array of int\n", offset, bytes, capacity, size);
    case CollectionPop:
    {
        const StyxIntVariable variable = {fid, pop_value, NULL};
        *size = capacity;
        return styxintvariableread(&variable, offset, bytes, size);
    }
    case CollectionValue:
        *size = capacity;
        return styxintarrayread(array, fid->generation, offset, bytes, size);
    default:
        return 0;
    }
}


/* EN: Decode a four-byte unsigned big-endian word.
 * RU: Разобрать четырёхбайтовое беззнаковое слово в сетевом порядке. */
static uint32_t word(const unsigned char *bytes)
{
    return (uint32_t)bytes[0] << 24 | (uint32_t)bytes[1] << 16 |
        (uint32_t)bytes[2] << 8 | bytes[3];
}


/* EN: Commit one insertion or key creation only with no live selector.
 * RU: Выполнить одну вставку или создание ключа только без активного селектора. */
static int insert(CollectionFid *fid, uint64_t offset, const unsigned char *bytes, size_t size)
{
    const unsigned operation = fid->entry->collection_op;
    StyxIntArray *array = fid->entry->collection;
    int append = operation == CollectionPush;
    if (offset || !bytes || size != (append ? 4U : 8U) || fid->completed || array->client ||
        array->length == UINT32_MAX)
    {
        return 0;
    }
    uint32_t index = append ? array->length : word(bytes);
    uint32_t bits = word(bytes + (append ? 0 : 4));
    int32_t value = bits <= INT32_MAX ? (int32_t)bits : -1 - (int32_t)(UINT32_MAX - bits);
    int accepted = operation == CollectionAdd ?
        array->contains(array->state, index) == 0 && array->add(array->state, index, value) == 1 :
        index <= array->length && array->insert(array->state, index, value) == 1;
    if (!accepted)
    {
        return 0;
    }
    array->length++;
    fid->completed = 1;
    return 1;
}


/* EN: Route one complete write to its selector, value, or mutation callback.
 * RU: Передать полную запись селектору, значению или колбэку мутации. */
static int write_collection(void *cookie, uint64_t offset, const void *bytes, size_t size, size_t *written)
{
    CollectionFid *fid = cookie;
    StyxIntArray *array = fid->entry->collection;
    unsigned operation = fid->entry->collection_op;
    int valid = operation == CollectionIndex ? styxintarrayselect(array, fid->generation, offset, bytes, size) :
        operation == CollectionValue ? styxintarraywrite(array, fid->generation, offset, bytes, size) :
        insert(fid, offset, bytes, size);
    if (valid)
    {
        *written = size;
    }
    return valid;
}


/* EN/RU: See collection_internal.h. / См. collection_internal.h. */
void styxrpc_collection_bind(StyxRpcRegistration *entry, StyxRpcPrograms *owner,
    StyxIntArray *array, CollectionOperation operation)
{
    entry->collection = array;
    entry->collection_owner = owner;
    entry->collection_op = operation;
    entry->resource = (StyxRpcResource){entry, open_collection, read_collection, write_collection, close_collection};
}
