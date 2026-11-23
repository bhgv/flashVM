/* Copyright (c) 2026 bhgv
 * EN: Encode and decode the ordinary Dis managed reference profile and copy-back.
 * RU: Кодирование и декодирование обычного профиля ссылок Dis и обратной записи.
 */
#include "managed_values.h"
#include "hybrid.h"
#include "flashvm/execution.h"
#include <stdlib.h>
#include <string.h>


/* EN: Project one flat ref token to the exact list of its field types.
 * RU: Преобразовать плоский токен ref в точный список типов полей. */
unsigned fvm_styxrpc_ref_fields(const char *type, int graph, char fields[STYXRPC_MANAGED_SHAPE])
{
    const char *end = fvm_styxrpc_type_end(type);
    const char *field;
    unsigned count = 0;
    size_t length;
    if (!end || *type != 'r' || end - type <= 3 || end - type >= STYXRPC_MANAGED_SHAPE)
    {
        return 0;
    }
    length = (size_t)(end - type) - 3;
    if (!graph && memchr(type + 2, 'r', length))
    {
        return 0;
    }
    fields[0] = '(';
    memcpy(fields + 1, type + 2, length);
    fields[length + 1] = ')';
    fields[length + 2] = 0;
    for (field = fields + 1; *field != ')'; field = fvm_styxrpc_type_end(field))
    {
        ++count;
    }
    return count <= FVM_STYXRPC_REF_FIELDS ? count : 0;
}


/* EN: Require aliases to use the same exact compiled type token.
 * RU: Требовать одинаковый точный токен типа для алиасов. */
static int same_type(const char *left, const char *right)
{
    size_t length = (size_t)(fvm_styxrpc_type_end(left) - left);
    return length == (size_t)(fvm_styxrpc_type_end(right) - right) && !memcmp(left, right, length);
}


/* EN: Retain a token that may have come from a temporary nested field schema.
 * RU: Сохранить токен, который мог происходить из временной вложенной схемы. */
static void save_type(FVM_StyxRef *ref, const char *type)
{
    size_t size = (size_t)(fvm_styxrpc_type_end(type) - type);
    memcpy(ref->type, type, size);
    ref->type[size] = 0;
}


/* EN: Track an ordinary argument position; graph identity ordinals are independent.
 * RU: Учесть позицию обычного аргумента; номера идентификаторов графа независимы. */
static int admit_slot(FVM_StyxRefs *refs)
{
    if (refs->graph)
    {
        return 1;
    }
    if (refs->slots == FVM_STYXRPC_REF_SLOTS)
    {
        return 0;
    }
    ++refs->slots;
    return 1;
}


/* [GC:] EN: Snapshot before any nested encoding or allocation can collect the fields.
 * RU: Снять исходные поля до вложенного кодирования и выделения памяти. */
static int snapshot(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRef *ref,
    const FVM_Value *value, unsigned count)
{
    unsigned i;
    if (!count || !fvm_styxrpc_ref_snapshot(context, value->object, count, ref->original) ||
        fvm_gc_roots_add(roots, value) != FVM_OK)
    {
        return 0;
    }
    ref->object = value->object;
    ref->count = count;
    for (i = 0; i < count; ++i)
    {
        if (fvm_gc_roots_add(roots, ref->original + i) != FVM_OK)
        {
            return 0;
        }
    }
    return 1;
}


/* [GC:] EN: Encode one owned table snapshot after pinning the script identity.
 * RU: Закодировать снимок таблицы после удержания идентификатора AS. */
static int encode_table(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRef *ref,
    const FVM_Value *value, unsigned char id, unsigned char *bytes, size_t capacity, size_t *used)
{
    StyxRpcHybrid *table = fvm_styxrpc_hybrid_lookup(context, value->object);
    unsigned char *snapshot_bytes;
    size_t size;
    int ok;
    if (!table || !capacity || fvm_gc_roots_add(roots, value) != FVM_OK)
    {
        return 0;
    }
    snapshot_bytes = malloc(capacity);
    if (!snapshot_bytes)
    {
        return 0;
    }
    ok = styxrpc_hybrid_snapshot(table, snapshot_bytes, capacity, &size) &&
        fvm_srecord_write_pair(STYXRPC_VALUE_REF, &id, 1, snapshot_bytes, size, bytes, capacity, used) == FVM_S_OK;
    free(snapshot_bytes);
    if (ok)
    {
        ref->object = value->object;
        save_type(ref, "h");
    }
    return ok;
}


/* [GC:] EN/RU: See managed_refs.h. / См. managed_refs.h. */
int fvm_styxrpc_ref_encode(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRefs *refs,
    const char *type, const FVM_Value *value, unsigned char *bytes, size_t capacity, size_t *used)
{
    FVM_StyxRef *ref;
    char fields[STYXRPC_MANAGED_SHAPE];
    unsigned count, i;
    unsigned char id;
    unsigned char *body;
    size_t size, room;
    int ok;
    if (!admit_slot(refs))
    {
        return 0;
    }
    if (value->type == FVM_VALUE_NULL)
    {
        return fvm_srecord_write(STYXRPC_VALUE_NIL, NULL, 0, bytes, capacity, used) == FVM_S_OK;
    }
    if (value->type != FVM_VALUE_OBJECT)
    {
        return 0;
    }
    for (i = 0; i < refs->count; ++i)
    {
        ref = refs->definitions + i;
        if (ref->object == value->object)
        {
            id = (unsigned char)i;
            return ref->type[0] && same_type(type, ref->type) &&
                fvm_srecord_write(STYXRPC_VALUE_REF, &id, 1, bytes, capacity, used) == FVM_S_OK;
        }
    }
    if (refs->count == refs->capacity)
    {
        return 0;
    }
    id = (unsigned char)refs->count;
    ref = refs->definitions + refs->count++;
    if (*type == 'h')
    {
        return encode_table(context, roots, ref, value, id, bytes, capacity, used);
    }
    count = fvm_styxrpc_ref_fields(type, refs->graph, fields);
    if (!snapshot(context, roots, ref, value, count))
    {
        return 0;
    }
    room = refs->graph ? capacity : STYXRPC_MANAGED_LIMIT - refs->used;
    body = refs->graph ? malloc(room ? room : 1) : refs->storage + refs->used;
    if (!body)
    {
        return 0;
    }
    ok = fvm_styxrpc_values_encode(context, roots, refs->graph ? refs : NULL, fields, ref->original, count,
        body, room, &size) &&
        fvm_srecord_write_pair(STYXRPC_VALUE_REF, &id, 1, body, size, bytes, capacity, used) == FVM_S_OK;
    if (refs->graph)
    {
        free(body);
    }
    else if (ok)
    {
        ref->before.payload = body;
        ref->before.size = size;
        ref->slot = refs->slots - 1;
        refs->used += size;
    }
    if (ok)
    {
        save_type(ref, type);
    }
    return ok;
}


/* [GC:] EN: Reconstruct a table locally and retain its owner before publishing the ID.
 * RU: Восстановить таблицу локально, удержать владельца и опубликовать ID. */
static int decode_table(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRef *ref,
    const FVM_SRecord *node, FVM_Value *value)
{
    StyxRpcHybrid *table = styxrpc_hybrid_open(node->payload + 1, node->size - 1);
    if (!table)
    {
        return 0;
    }
    if (fvm_styxrpc_hybrid_adopt(context, table, value) != FVM_OK)
    {
        styxrpc_hybrid_close(table);
        return 0;
    }
    if (fvm_gc_roots_add(roots, value) != FVM_OK)
    {
        return 0;
    }
    ref->record = *node;
    ref->object = value->object;
    save_type(ref, "h");
    return 1;
}


/* [GC:] EN/RU: See managed_refs.h. / См. managed_refs.h. */
int fvm_styxrpc_ref_decode(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRefs *refs,
    const char *type, const FVM_SRecord *node, FVM_Value *value)
{
    FVM_StyxRef *ref;
    char fields[STYXRPC_MANAGED_SHAPE];
    unsigned index, count, actual;
    if (!admit_slot(refs))
    {
        return 0;
    }
    fvm_value_undefined(value);
    if (node->tag == STYXRPC_VALUE_NIL && !node->size)
    {
        value->type = FVM_VALUE_NULL;
        return 1;
    }
    if (node->tag != STYXRPC_VALUE_REF || !node->size || node->payload[0] > refs->count || node->payload[0] >= refs->capacity)
    {
        return 0;
    }
    index = node->payload[0];
    ref = refs->definitions + index;
    if (index < refs->count)
    {
        if (node->size != 1 || !ref->type[0] || !same_type(type, ref->type))
        {
            return 0;
        }
        value->type = FVM_VALUE_OBJECT;
        value->object = ref->object;
        return 1;
    }
    if (*type == 'h')
    {
        ++refs->count;
        return decode_table(context, roots, ref, node, value);
    }
    count = fvm_styxrpc_ref_fields(type, refs->graph, fields);
    if (!count)
    {
        return 0;
    }
    ++refs->count;
    ref->record = *node;
    ref->before.payload = node->payload + 1;
    ref->before.size = node->size - 1;
    if (!fvm_styxrpc_values_decode(context, roots, refs->graph ? refs : NULL, fields, &ref->before,
        ref->original, FVM_STYXRPC_REF_FIELDS, &actual) || actual != count ||
        fvm_exec_literal_array(context, count, ref->original, value) != FVM_OK ||
        fvm_gc_roots_add(roots, value) != FVM_OK)
    {
        return 0;
    }
    save_type(ref, type);
    ref->object = value->object;
    ref->count = count;
    if (!refs->graph)
    {
        ref->slot = refs->slots - 1;
    }
    return 1;
}


/* [GC:] EN/RU: See managed_refs.h. / См. managed_refs.h. */
int fvm_styxrpc_refs_reply(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRefs *refs,
    unsigned char *bytes, size_t capacity, StyxRpcOutput *output)
{
    unsigned char *scratch;
    size_t written = 0;
    unsigned char id = 0;
    unsigned i;
    int ok = 1;
    if (!refs->count)
    {
        return 1;
    }
    scratch = malloc(STYXRPC_MANAGED_LIMIT);
    if (!scratch)
    {
        return 0;
    }
    for (i = 0; ok && i < refs->count; ++i)
    {
        const FVM_StyxRef *ref = refs->definitions + i;
        char fields[STYXRPC_MANAGED_SHAPE];
        FVM_Value current[FVM_STYXRPC_REF_FIELDS];
        size_t size, extent;
        unsigned field;
        ok = fvm_styxrpc_ref_fields(ref->type, 0, fields) && fvm_styxrpc_ref_snapshot(context, ref->object, ref->count, current);
        for (field = 0; ok && field < ref->count; ++field)
        {
            ok = fvm_gc_roots_add(roots, current + field) == FVM_OK;
        }
        ok = ok && fvm_styxrpc_values_encode(context, roots, NULL, fields, current, ref->count,
            scratch, STYXRPC_MANAGED_LIMIT, &size);
        if (!ok || (size == ref->before.size && !memcmp(scratch, ref->before.payload, size)))
        {
            continue;
        }
        ok = fvm_srecord_write_pair(STYXRPC_VALUE_REF, &id, 1, scratch, size,
            bytes + written, capacity - written, &extent) == FVM_S_OK;
        if (ok)
        {
            output->changed |= (uint8_t)(1U << ref->slot);
            written += extent;
            ++id;
        }
    }
    free(scratch);
    output->updates = bytes;
    output->update_size = written;
    return ok;
}


/* EN: Compare canonical field records to retain unchanged local objects and strings.
 * RU: Сравнить канонические поля и сохранить неизменившиеся объекты и строки. */
static int changed_fields(FVM_StyxRef *ref, const FVM_SRecord *after)
{
    FVM_SRecord left, right, child;
    unsigned i;
    size_t a, b;
    left = ref->before;
    right = *after;
    for (i = 0; i < ref->count; ++i)
    {
        if (fvm_srecord_read(left.payload, left.size, STYXRPC_MANAGED_LIMIT, &child, &a) != FVM_S_OK ||
            fvm_srecord_read(right.payload, right.size, STYXRPC_MANAGED_LIMIT, &child, &b) != FVM_S_OK)
        {
            return 0;
        }
        if (a != b || memcmp(left.payload, right.payload, a))
        {
            ref->changed |= 1U << i;
        }
        left.payload += a;
        left.size -= a;
        right.payload += b;
        right.size -= b;
    }
    return !left.size && !right.size;
}


/* [GC:] EN/RU: See managed_refs.h. / См. managed_refs.h. */
int fvm_styxrpc_refs_stage(FVM_Context *context, FVM_GCRootScope *roots, FVM_StyxRefs *refs,
    const StyxRpcManagedReply *reply)
{
    FVM_SRecord tail = reply->updates;
    unsigned i, mask = reply->changed;
    for (i = 0; i < refs->count; ++i)
    {
        FVM_StyxRef *ref = refs->definitions + i;
        if (mask & (1U << ref->slot))
        {
            FVM_SRecord node;
            char fields[STYXRPC_MANAGED_SHAPE];
            size_t used;
            unsigned count;
            if (!fvm_styxrpc_ref_fields(ref->type, 0, fields) ||
                fvm_srecord_read(tail.payload, tail.size, STYXRPC_MANAGED_LIMIT, &node, &used) != FVM_S_OK ||
                node.tag != STYXRPC_VALUE_REF || node.size < 2)
            {
                return 0;
            }
            ++node.payload;
            --node.size;
            if (!changed_fields(ref, &node) ||
                !fvm_styxrpc_values_decode(context, roots, NULL, fields, &node, ref->updated,
                    FVM_STYXRPC_REF_FIELDS, &count) || count != ref->count)
            {
                return 0;
            }
            tail.payload += used;
            tail.size -= used;
            mask &= ~(1U << ref->slot);
        }
    }
    return !mask && !tail.size;
}
