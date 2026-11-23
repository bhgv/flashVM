/* Copyright (c) 2026 bhgv
 * EN: Application-native dense and keyed buffers behind the hybrid table wire files.
 * RU: Нативные буферы позиционной и ключевой частей за файлами гибридной таблицы.
 */
#include "hybrid.h"
#include <string.h>


/* EN: Find a native value in the separate positional or key namespace.
 * RU: Найти нативное значение в позиционном либо ключевом пространстве. */
static int64_t *find(Hybrid *table, const StyxRpcBigKey *key)
{
    if (!key->text && key->index >= 0 && (uint64_t)key->index < table->length)
    {
        return &table->dense[key->index];
    }
    for (size_t i = 0; i < table->count; ++i)
    {
        HybridEntry *entry = &table->sparse[i];
        if (key->text == entry->key.text && (key->text ?
            key->size == entry->key.size && (!key->size || !memcmp(key->bytes, entry->text, key->size)) :
            key->index == entry->key.index))
        {
            return &entry->value;
        }
    }
    return NULL;
}


/* EN: Append one distinct sparse or text key to the bounded fixture state.
 * RU: Добавить отдельный разреженный или текстовый ключ в ограниченное состояние примера. */
static int add(Hybrid *table, StyxRpcBigKey key, int64_t value)
{
    if (table->count == sizeof(table->sparse) / sizeof(*table->sparse) ||
        (key.text ? key.size > sizeof(table->sparse[0].text) :
            key.index < 0 || (uint64_t)key.index <= table->length) ||
        find(table, &key))
    {
        return 0;
    }
    HybridEntry *entry = &table->sparse[table->count++];
    entry->key = key;
    entry->value = value;
    if (key.text)
    {
        memcpy(entry->text, key.bytes, key.size);
        entry->key.bytes = entry->text;
    }
    return 1;
}


/* EN: Insert one value in the live dense prefix without affecting sparse fixture keys.
 * RU: Вставить одно значение в плотную часть, сохранив разреженные ключи примера. */
static int insert(Hybrid *table, int64_t position, int64_t value)
{
    if (position < 0 || (uint64_t)position > table->length ||
        table->length == sizeof(table->dense) / sizeof(*table->dense))
    {
        return 0;
    }
    for (size_t i = 0; i < table->count; ++i)
    {
        if (!table->sparse[i].key.text && (uint64_t)table->sparse[i].key.index == table->length)
        {
            return 0;
        }
    }
    memmove(table->dense + position + 1, table->dense + position,
        (table->length - (size_t)position) * sizeof(*table->dense));
    table->dense[position] = value;
    table->length++;
    return 1;
}


/* EN: Process one complete command on the dispatcher thread.
 * RU: Обработать одну полную команду в потоке диспетчера. */
static int apply(void *context, StyxRpcBigRequest *request)
{
    Hybrid *table = context;
    switch (request->operation)
    {
    case STYXRPC_BIG_LENGTH:
        request->count = table->length;
        return 1;
    case STYXRPC_BIG_COUNT:
        request->count = table->length + table->count;
        return 1;
    case STYXRPC_BIG_INSERT:
        return insert(table, request->position, request->value);
    case STYXRPC_BIG_PUSH:
        return insert(table, (int64_t)table->length, request->value);
    case STYXRPC_BIG_POP:
        if (!table->length)
        {
            return 0;
        }
        request->value = table->dense[--table->length];
        return 1;
    case STYXRPC_BIG_ADD:
        return add(table, request->key, request->value);
    case STYXRPC_BIG_SELECT:
        if (!request->key.text && request->key.index < 0)
        {
            request->key.index += (int64_t)table->length;
            if (request->key.index < 0)
            {
                return 0;
            }
        }
        return find(table, &request->key) != NULL;
    case STYXRPC_BIG_GET:
    case STYXRPC_BIG_UPDATE:
    {
        int64_t *value = find(table, &request->key);
        if (!value)
        {
            return 0;
        }
        if (request->operation == STYXRPC_BIG_GET)
        {
            request->value = *value;
        }
        else
        {
            *value = request->value;
        }
        return 1;
    }
    }
    return 0;
}


/* EN/RU: See hybrid.h. / См. hybrid.h. */
int register_hybrid(StyxRpcPrograms *programs, uint32_t pid, Hybrid *state)
{
    state->table = (StyxRpcBigTable){state, apply, 1};
    state->dense[0] = 10;
    state->dense[1] = 20;
    state->length = 2;
    const StyxRpcBigKey text = {.text = 1, .bytes = "1", .size = 1};
    const StyxRpcBigKey sparse = {.index = 5};
    return add(state, text, 90) && add(state, sparse, 50) &&
        styxrpc_program_bigtable(programs, pid, "hybrid", &state->table);
}
