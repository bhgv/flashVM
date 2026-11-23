/* Copyright (c) 2026 bhgv
 * EN: Stock hybrid table wire commands with revocable, per-open key selection.
 * RU: Штатные команды гибридной таблицы с отзываемым выбором на открытом fid.
 */
#include "bigtable_internal.h"
#include "flashvm/utf8.h"
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

typedef struct Selection
{
    StyxRpcBigKey key;                 /* Canonical numeric key or private text bytes. */
    char text[STYXRPC_TABLE_KEY_LIMIT]; /* Retained through the last value fid. */
    size_t pins;                       /* Selector plus any dependent value fids. */
    int live;                          /* Revoked by selector close. */
    int selected;                      /* Complete admitted key available. */
} Selection;

typedef struct BigFid
{
    StyxRpcRegistration *directory; /* Stable publication for one table. */
    BigNode operation;               /* Exact command from Topen. */
    Selection *selection;            /* Owned pin, if index or value. */
    size_t length;                   /* Cached metadata, read value or pop bytes. */
    int completed;                   /* One-shot command already executed. */
    unsigned char snapshot[32];      /* Read cursor content, repeatable by offset. */
} BigFid;


/* EN: Decode a signed network-order word without implementation-defined narrowing.
 * RU: Декодировать знаковое слово в сетевом порядке без сужения с неопределённым результатом. */
static int64_t word(const unsigned char *bytes)
{
    uint64_t bits = 0;
    for (unsigned i = 0; i < 8; ++i)
    {
        bits = (bits << 8) | bytes[i];
    }
    return bits <= INT64_MAX ? (int64_t)bits : -1 - (int64_t)(UINT64_MAX - bits);
}


/* EN: Decode the stock numeric (0 plus big) or text (1 plus counted UTF-8) key.
 * RU: Декодировать штатный числовой (0 плюс big) или текстовый ключ (1 плюс UTF-8). */
static int decode_key(const unsigned char *bytes, size_t size, StyxRpcBigKey *key)
{
    if (bytes && size == 9 && bytes[0] == 0)
    {
        *key = (StyxRpcBigKey){.index = word(bytes + 1)};
        return 1;
    }
    if (bytes && size && bytes[0] == 1 && size <= STYXRPC_TABLE_KEY_LIMIT + 1 &&
        fvm_utf8_validate((const char *)bytes + 1, size - 1, NULL) == FVM_UTF8_OK)
    {
        *key = (StyxRpcBigKey){.text = 1, .bytes = (const char *)bytes + 1, .size = size - 1};
        return 1;
    }
    return 0;
}


/* EN: Allocate a new selector or retain the exact published key for this .val.
 * RU: Выделить селектор либо удержать точный опубликованный ключ для .val. */
static int pin_selection(BigFid *fid)
{
    Selection *current = fid->directory->table_selection;
    if (fid->operation == BigIndex)
    {
        if (current)
        {
            return 0;
        }
        current = calloc(1, sizeof(*current));
        if (!current)
        {
            return 0;
        }
        current->live = 1;
        fid->directory->table_selection = current;
    }
    else if (!current || !current->selected || current->pins == SIZE_MAX)
    {
        return 0;
    }
    current->pins++;
    fid->selection = current;
    return 1;
}


/* EN: Pin one exact file kind on the serialized tree owner.
 * RU: Закрепить точный тип файла в сериализованном диспетчере дерева. */
static int open_table(void *context, unsigned access, void **cookie)
{
    StyxRpcRegistration *entry = context;
    (void)access;
    BigFid *fid = calloc(1, sizeof(*fid));
    if (!fid)
    {
        return 0;
    }
    fid->directory = entry->table_directory;
    fid->operation = (BigNode)entry->collection_op;
    if ((fid->operation == BigIndex || fid->operation == BigValue) && !pin_selection(fid))
    {
        free(fid);
        return 0;
    }
    *cookie = fid;
    return 1;
}


/* EN: Revoke dependent fids only when their owning selector closes.
 * RU: Отозвать зависимые fid только при закрытии их селектора. */
static void close_table(void *cookie)
{
    BigFid *fid = cookie;
    Selection *selected = fid->selection;
    if (selected)
    {
        if (fid->operation == BigIndex)
        {
            fid->directory->table_selection = NULL;
            selected->live = 0;
        }
        if (!--selected->pins)
        {
            free(selected);
        }
    }
    free(fid);
}


/* EN: Invoke the table owner's one atomic callback.
 * RU: Вызвать одну атомарную операцию владельца таблицы. */
static int apply(BigFid *fid, StyxRpcBigRequest *request)
{
    const StyxRpcBigTable *table = fid->directory->table_spec;
    return table->apply(table->context, request) == 1;
}


/* EN: Cache one read snapshot without replaying a pop on subsequent reads.
 * RU: Сохранить снимок чтения, чтобы повторное чтение не повторило pop. */
static int snapshot(BigFid *fid)
{
    if (fid->operation == BigType)
    {
        static const char type[] = "table of big\n";
        memcpy(fid->snapshot, type, sizeof(type) - 1);
        fid->length = sizeof(type) - 1;
        return 1;
    }
    StyxRpcBigRequest request = {.operation = fid->operation == BigValue ? STYXRPC_BIG_GET :
        fid->operation == BigPop ? STYXRPC_BIG_POP : fid->operation == BigLength ? STYXRPC_BIG_LENGTH : STYXRPC_BIG_COUNT};
    if (fid->selection)
    {
        request.key = fid->selection->key;
    }
    if (!apply(fid, &request))
    {
        return 0;
    }
    if (fid->operation == BigValue || fid->operation == BigPop)
    {
        uint64_t bits = (uint64_t)request.value;
        for (unsigned i = 0; i < 8; ++i)
        {
            fid->snapshot[i] = (unsigned char)(bits >> (56 - 8 * i));
        }
        fid->length = 8;
    }
    else
    {
        fid->length = (size_t)snprintf((char *)fid->snapshot, sizeof(fid->snapshot), "%" PRIu64 "\n", request.count);
    }
    return 1;
}


/* EN: Return a byte slice on one live opened file.
 * RU: Вернуть фрагмент байтов одного живого открытого файла. */
static int read_table(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    BigFid *fid = cookie;
    *size = 0;
    if (fid->selection && !fid->selection->live)
    {
        return 0;
    }
    if (!fid->completed && capacity)
    {
        if (!snapshot(fid))
        {
            return 0;
        }
        fid->completed = 1;
    }
    if (offset < fid->length)
    {
        size_t remaining = fid->length - (size_t)offset;
        *size = capacity < remaining ? capacity : remaining;
        if (*size)
        {
            memcpy(bytes, fid->snapshot + (size_t)offset, *size);
        }
    }
    return 1;
}


/* EN: Install an existing, canonical key only after the owner's SELECT succeeds.
 * RU: Установить существующий канонический ключ после успешного SELECT владельца. */
static int select_key(BigFid *fid, const unsigned char *bytes, size_t size)
{
    StyxRpcBigRequest request = {.operation = STYXRPC_BIG_SELECT};
    StyxRpcBigKey supplied;
    if (!decode_key(bytes, size, &supplied))
    {
        return 0;
    }
    request.key = supplied;
    if (!apply(fid, &request) || (!supplied.text && request.key.index < 0))
    {
        return 0;
    }
    Selection *selection = fid->selection;
    if (supplied.text)
    {
        memcpy(selection->text, supplied.bytes, supplied.size);
        supplied.bytes = selection->text;
    }
    else
    {
        supplied.index = request.key.index;
    }
    selection->key = supplied;
    selection->selected = 1;
    return 1;
}


/* EN: Decode one mutation; value updates use the fid's pinned key.
 * RU: Декодировать одну мутацию. Обновление использует закреплённый ключ fid. */
static int mutate(BigFid *fid, const unsigned char *bytes, size_t size)
{
    StyxRpcBigRequest request = {0};
    switch (fid->operation)
    {
    case BigIndex:
        return select_key(fid, bytes, size);
    case BigValue:
        if (size != 8)
        {
            return 0;
        }
        request.operation = STYXRPC_BIG_UPDATE;
        request.key = fid->selection->key;
        break;
    case BigPush:
        if (size != 8)
        {
            return 0;
        }
        request.operation = STYXRPC_BIG_PUSH;
        break;
    case BigInsert:
        if (size != 16)
        {
            return 0;
        }
        request.operation = STYXRPC_BIG_INSERT;
        request.position = word(bytes);
        break;
    case BigAdd:
        if (size < 9 || !decode_key(bytes, size - 8, &request.key))
        {
            return 0;
        }
        request.operation = STYXRPC_BIG_ADD;
        break;
    default:
        return 0;
    }
    request.value = word(bytes + size - 8);
    return apply(fid, &request);
}


/* EN: Perform one acknowledged command, preserving the exact written value in its cache.
 * RU: Выполнить подтверждённую команду и сохранить записанное значение в снимке fid. */
static int write_table(void *cookie, uint64_t offset, const void *bytes, size_t size, size_t *written)
{
    BigFid *fid = cookie;
    if (offset || !bytes || (fid->selection && !fid->selection->live) ||
        (fid->operation != BigValue && fid->completed) || !mutate(fid, bytes, size))
    {
        return 0;
    }
    fid->completed = 1;
    if (fid->operation == BigValue)
    {
        memcpy(fid->snapshot, bytes, size);
        fid->length = size;
    }
    *written = size;
    return 1;
}


/* EN/RU: See bigtable_internal.h. / См. bigtable_internal.h. */
void styxrpc_bigtable_bind(StyxRpcRegistration *entry, StyxRpcRegistration *directory, BigNode operation)
{
    entry->table_directory = directory;
    entry->collection_op = (unsigned)operation;
    entry->resource = (StyxRpcResource){entry, open_table, read_table, write_table, close_table};
}
