/* Copyright (c) 2026 bhgv
 * EN: Bounded portable scalar history snapshots for explicitly negotiated transport envelopes.
 * RU: Bounded portable scalar snapshots history для явно согласованных transport envelopes.
 */
#include "internal.h"
#include <stdlib.h>
#include <string.h>
#define WALK_WIRE_VERSION 1U /* Scalar snapshot format inside an owning transport's control record. */
#define WALK_WIRE_ENTRY 44U /* Eight identity limbs, two node limbs, operation and two string lengths. */


/* EN/RU: Write validated unsigned32 limb in network order to four borrowed bytes; no native layout copy.
 * Записать validated unsigned32 limb в четыре borrowed bytes в network order; без native layout copy.
 */
static void put(unsigned char *bytes, unsigned long value)
{
    for (unsigned i = 0; i < 4; i++)
    {
        bytes[i] = (unsigned char)(value >> (24 - i * 8));
    }
}


/* EN/RU: Read one explicit unsigned32 limb from four borrowed bytes; no narrowing of a native pointer.
 * Прочитать unsigned32 limb из четырёх borrowed bytes; без сужения native pointer.
 */
static unsigned long get(const unsigned char *bytes)
{
    unsigned long value = 0;
    for (unsigned i = 0; i < 4; i++)
    {
        value = (value << 8) | bytes[i];
    }
    return value;
}


/* EN/RU: See walk.h; strings are borrowed from this native owner's stable active scopes.
 * См. walk.h; strings borrowed из stable active scopes этого native owner.
 */
size_t walk_export_size(void)
{
    size_t size = 4;
    for (size_t i = 0; i < walk_stack.count; i++)
    {
        WalkEntry *entry = &walk_stack.entries[i];
        if (!walk_words(entry->entity.domain, 1))
        {
            return 0;
        }
        size += WALK_WIRE_ENTRY + strlen(entry->remaining) + strlen(entry->detail);
    }
    return size <= WALK_WIRE_LIMIT ? size : 0;
}


/* EN/RU: See walk.h; validate complete size and exportable domains before publishing bytes/used.
 * См. walk.h; проверить полный размер и exportable domains до выдачи bytes/used.
 */
int walk_export(void *output, size_t capacity, size_t *used)
{
    WalkStack *stack = &walk_stack;
    size_t size = walk_export_size();
    if (!output || !used || !size)
    {
        return 0;
    }
    if (size > capacity || size > WALK_WIRE_LIMIT)
    {
        return 0;
    }
    unsigned char *bytes = output;
    bytes[0] = WALK_WIRE_VERSION;
    bytes[1] = 0;
    bytes[2] = (unsigned char)(stack->count >> 8);
    bytes[3] = (unsigned char)stack->count;
    size_t position = 4;
    for (size_t i = 0; i < stack->count; i++)
    {
        WalkEntry *entry = &stack->entries[i];
        for (unsigned j = 0; j < 4; j++)
        {
            put(bytes + position, entry->entity.domain[j]);
            put(bytes + position + 16, entry->entity.object[j]);
            position += 4;
        }
        position += 16;
        put(bytes + position, entry->high);
        put(bytes + position + 4, entry->low);
        position += 8;
        size_t path = strlen(entry->remaining);
        size_t detail = strlen(entry->detail);
        bytes[position++] = (unsigned char)entry->operation;
        bytes[position++] = (unsigned char)(path >> 8);
        bytes[position++] = (unsigned char)path;
        bytes[position++] = (unsigned char)detail;
        memcpy(bytes + position, entry->remaining, path);
        position += path;
        memcpy(bytes + position, entry->detail, detail);
        position += detail;
    }
    *used = position;
    return 1;
}


/* EN/RU: See walk.h; copied strings survive original receive-buffer reuse, rollback pops only this imported suffix.
 * См. walk.h; copied strings переживают reuse receive-buffer, rollback снимает только imported suffix.
 */
int walk_import(WalkScope *scope, const void *input, size_t size)
{
    const unsigned char *bytes = input;
    if (!scope || scope->active || !bytes || size < 4 || size > WALK_WIRE_LIMIT ||
        bytes[0] != WALK_WIRE_VERSION || bytes[1])
    {
        return WALK_INVALID;
    }
    unsigned count = ((unsigned)bytes[2] << 8) | bytes[3];
    if (count > WALK_DEPTH_LIMIT || count + walk_stack.count > WALK_DEPTH_LIMIT)
    {
        return WALK_LIMIT;
    }
    char *storage = count ? malloc(size + count * 2) : NULL;
    if (count && !storage)
    {
        return WALK_NOMEM;
    }
    scope->owner = &walk_stack;
    scope->mark = walk_stack.count;
    scope->entered = walk_stack.count;
    scope->owned = storage;
    scope->active = 1;
    size_t position = 4;
    size_t copied = 0;
    int result = WALK_INVALID;
    for (unsigned i = 0; i < count; i++)
    {
        if (size - position < WALK_WIRE_ENTRY)
        {
            goto failed;
        }
        WalkEntity entity;
        for (unsigned j = 0; j < 4; j++)
        {
            entity.domain[j] = get(bytes + position);
            entity.object[j] = get(bytes + position + 16);
            position += 4;
        }
        position += 16;
        unsigned long high = get(bytes + position);
        unsigned long low = get(bytes + position + 4);
        position += 8;
        unsigned operation = bytes[position++];
        unsigned path = (unsigned)bytes[position] << 8 | bytes[position + 1];
        position += 2;
        unsigned detail = bytes[position++];
        if (!walk_words(entity.domain, 1) || path >= WALK_PATH_LIMIT || detail >= WALK_DETAIL_LIMIT ||
            path + detail > size - position || memchr(bytes + position, 0, path + detail))
        {
            goto failed;
        }
        char *name = storage + copied;
        memcpy(name, bytes + position, path);
        name[path] = 0;
        copied += path + 1;
        position += path;
        char *mode = storage + copied;
        memcpy(mode, bytes + position, detail);
        mode[detail] = 0;
        copied += detail + 1;
        position += detail;
        WalkScope entry = {0};
        result = walk_enter(&entry, &entity, high, low, operation, name, mode);
        scope->entered = walk_stack.count;
        if (result != WALK_OK)
        {
            goto failed;
        }
    }
    if (position != size)
    {
        result = WALK_INVALID;
        goto failed;
    }
    return WALK_OK;
failed:
    if (!walk_leave(scope))
    {
        abort();
    }
    return result == WALK_OK ? WALK_INVALID : result;
}
