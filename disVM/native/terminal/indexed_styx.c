/* Copyright (c) 2026 bhgv
 * EN: Original Styx file callbacks for a callback-backed array.
 * RU: Файловые обработчики исходного Styx для массива с обратными вызовами.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "indexed_styx.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


enum
{
    ArrayIndex = 1, /* .idx selection. */
    ArrayValue = 2, /* .val selected element. */
    ArrayLength = 3, /* .len metadata. */
    ArrayType = 4, /* .type metadata. */
    ArrayInsert = 5, /* .ins indexed insertion. */
    ArrayPush = 6, /* .push append. */
    ArrayPop = 7, /* .pop removal and result. */
    TableAdd = 8 /* .add new key and value. */
};


/* EN/RU: Publish an optional mutation file with its exact direction.
 * Опубликовать необязательную операцию с её направлением доступа. */
static int add_mutations(Styxserver *server, Path base, const char *owner, const StyxIntArray *array)
{
    return (!array->insert || (styxaddfile(server, base, base + ArrayInsert, ".ins", 0222, (char *)owner) &&
        styxaddfile(server, base, base + ArrayPush, ".push", 0222, (char *)owner))) &&
        (!array->pop || styxaddfile(server, base, base + ArrayPop, ".pop", 0444, (char *)owner)) &&
        (!array->add || styxaddfile(server, base, base + TableAdd, ".add", 0222, (char *)owner));
}


/* EN/RU: Publish the array under parent using reserved consecutive qids.
 * Опубликовать массив под parent, используя зарезервированные qid подряд. */
int styxarrayfilepublish(StyxArrayFiles *files, Styxserver *server, Path parent, Path base,
    const char *name, const char *owner, StyxIntArray *array)
{
    if (!array || !array->load || (array->contains && (array->insert || array->pop)) ||
        (array->add && !array->contains))
    {
        return 0;
    }
    Path last = array->add ? TableAdd : array->pop ? ArrayPop : array->insert ? ArrayPush : ArrayType;
    if (!files || !server || !base || base >= UINT64_MAX - last || !name || !*name || strchr(name, '/') ||
        !strcmp(name, ".") || !strcmp(name, "..") || !owner || !*owner)
    {
        return 0;
    }
    for (Path qid = base; qid <= base + last; qid++)
    {
        if (styxfindfile(server, qid))
        {
            return 0;
        }
    }
    if (!styxadddir(server, parent, base, (char *)name, 0555, (char *)owner) ||
        !styxaddfile(server, base, base + ArrayIndex, ".idx", 0222, (char *)owner) ||
        !styxaddfile(server, base, base + ArrayValue, ".val", array->store ? 0666 : 0444, (char *)owner) ||
        !styxaddfile(server, base, base + ArrayLength, ".len", 0444, (char *)owner) ||
        !styxaddfile(server, base, base + ArrayType, ".type", 0444, (char *)owner) || !add_mutations(server, base, owner, array))
    {
        return 0;
    }
    *files = (StyxArrayFiles){server, base, array};
    return 1;
}


typedef struct Opened
{
    uint64_t generation; /* Selection held by this fid. */
    StyxIntArray *array; /* Borrowed array through fid retirement. */
    int completed; /* A mutation on this fid already committed. */
    int32_t popped; /* Cached successful .pop result. */
    int selector; /* Closing .idx retires the selection. */
} Opened;


/* EN/RU: Open a selector or value on the connection that owns the selection.
 * Открыть выбор или значение в соединении, которому принадлежит выбор. */
char *styxarrayfileopen(StyxArrayFiles *files, Qid *qid, int mode)
{
    if (qid->path == files->base || qid->path == files->base + ArrayLength || qid->path == files->base + ArrayType)
    {
        return mode == OREAD ? nil : Eperm;
    }
    int mutation = qid->path >= files->base + ArrayInsert && qid->path <= files->base + TableAdd;
    if (mutation && (qid->path == files->base + ArrayPop ? !files->array->pop || mode != OREAD :
        (qid->path == files->base + TableAdd ? !files->array->add : !files->array->insert) || mode != OWRITE))
    {
        return Eperm;
    }
    if (!mutation && qid->path != files->base + ArrayIndex && qid->path != files->base + ArrayValue)
    {
        return Ebadarg;
    }
    int selector = qid->path == files->base + ArrayIndex;
    if ((selector && mode != OWRITE) || (!mutation && !selector && mode != OREAD &&
        (!files->array->store || (mode != OWRITE && mode != ORDWR))))
    {
        return Eperm;
    }
    Opened *fid = calloc(1, sizeof(*fid));
    if (!fid)
    {
        return Enomem;
    }
    fid->selector = selector;
    fid->array = files->array;
    if (!mutation)
    {
        fid->generation = styxintarrayopen(files->array, styxclient(files->server), selector);
    }
    if ((!mutation && !fid->generation) || styxsetauxowned(files->server, fid, styxarrayfileclose) < 0)
    {
        if (selector)
        {
            styxintarrayclose(fid->array, fid->generation);
        }
        free(fid);
        return Eperm;
    }
    return nil;
}


/* EN/RU: Retire selectors on clunk or disconnect; consume the fid context.
 * Отозвать выбор при clunk или отключении и освободить контекст fid. */
void styxarrayfileclose(void *state)
{
    Opened *fid = state;
    if (fid->selector)
    {
        styxintarrayclose(fid->array, fid->generation);
    }
    free(fid);
}


/* EN/RU: Pop at most once per fid and replay only the cached result.
 * Извлечь не более одного элемента на fid, повторяя чтение сохранённого результата. */
static int pop_value(void *state, int32_t *value)
{
    Opened *fid = state;
    StyxIntArray *array = fid->array;
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


/* EN/RU: Read one unsigned network-order word from four borrowed bytes.
 * Прочитать одно беззнаковое слово из четырёх заимствованных байтов. */
static uint32_t word(const unsigned char *bytes)
{
    return ((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16) | ((uint32_t)bytes[2] << 8) | bytes[3];
}


/* EN/RU: Decode one insertion and commit only when no selection is open.
 * Декодировать одну вставку и выполнить её без открытого выбора. */
static int insert_value(Opened *fid, Path operation, const unsigned char *bytes, size_t size, uint64_t offset)
{
    StyxIntArray *array = fid->array;
    int append = operation == ArrayPush;
    if (offset || fid->completed || array->client || array->length == UINT32_MAX ||
        size != (append ? 4U : 8U))
    {
        return 0;
    }
    uint32_t index = array->length;
    if (!append)
    {
        index = word(bytes);
        bytes += 4;
    }
    uint32_t bits = word(bytes);
    int32_t value = bits <= INT32_MAX ? (int32_t)bits : -1 - (int32_t)(UINT32_MAX - bits);
    int accepted;
    if (operation == TableAdd)
    {
        accepted = array->contains(array->state, index) == 0 && array->add(array->state, index, value) == 1;
    }
    else
    {
        accepted = index <= array->length && array->insert(array->state, index, value) == 1;
    }
    if (!accepted)
    {
        return 0;
    }
    array->length++;
    fid->completed = 1;
    return 1;
}


/* EN/RU: Read the element selected by this value fid.
 * Прочитать элемент, выбранный этим fid значения. */
char *styxarrayfileread(StyxArrayFiles *files, Qid qid, char *bytes, ulong *count, vlong offset)
{
    if ((qid.path == files->base + ArrayLength || qid.path == files->base + ArrayType) && offset >= 0)
    {
        if (qid.path == files->base + ArrayType)
        {
            *count = styxreadstr(offset, bytes, *count, files->array->contains ? "table of int\n" : "array of int\n");
        }
        else
        {
            char length[16];
            snprintf(length, sizeof(length), "%" PRIu32 "\n", files->array->length);
            *count = styxreadstr(offset, bytes, *count, length);
        }
        return nil;
    }
    Opened *fid = styxgetaux(files->server);
    size_t size = *count;
    if (!fid || offset < 0)
    {
        return Ebadarg;
    }
    const StyxIntVariable pop = {fid, pop_value, NULL};
    int accepted = qid.path == files->base + ArrayPop ? styxintvariableread(&pop, offset, bytes, &size) :
        qid.path == files->base + ArrayValue && styxintarrayread(fid->array, fid->generation, offset, bytes, &size);
    if (!accepted)
    {
        return Ebadarg;
    }
    *count = size;
    return nil;
}


/* EN/RU: Write one index or update the selected value according to the opened file.
 * Записать индекс или заменить выбранное значение в зависимости от открытого файла. */
char *styxarrayfilewrite(StyxArrayFiles *files, Qid qid, char *bytes, ulong *count, vlong offset)
{
    Opened *fid = styxgetaux(files->server);
    if (!fid || offset < 0)
    {
        return Ebadarg;
    }
    if (qid.path == files->base + ArrayInsert || qid.path == files->base + ArrayPush || qid.path == files->base + TableAdd)
    {
        return insert_value(fid, qid.path - files->base,
            (const unsigned char *)bytes, *count, offset) ? nil : Ebadarg;
    }
    if (qid.path != files->base + ArrayIndex && qid.path != files->base + ArrayValue)
    {
        return Ebadarg;
    }
    int accepted = qid.path == files->base + ArrayIndex ? styxintarrayselect(fid->array, fid->generation, offset, bytes, *count) :
        styxintarraywrite(fid->array, fid->generation, offset, bytes, *count);
    return accepted ? nil : Ebadarg;
}
