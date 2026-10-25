/* Bounded directional metadata without allocation, native object addresses or global tag tables. */
#include "flashvm/sbindings.h"
#include <string.h>


/* Read a canonical unsigned 32-bit metadata field and advance a private cursor only on success. */
static FVM_SResult field_read(const unsigned char **cursor, size_t *left, unsigned long *out)
{
    FVM_SInteger number;
    size_t used;
    FVM_SResult result = fvm_sinteger_read(*cursor, *left, &number, &used);
    if (result != FVM_S_OK || number.negative || number.high)
    {
        return FVM_S_INVALID;
    }
    *out = number.low;
    *cursor += used;
    *left -= used;
    return FVM_S_OK;
}


/* Canonical field writer used only after the complete payload has passed validation. */
static size_t field_write(unsigned long value, unsigned char *out)
{
    FVM_SInteger number;
    size_t used = 0;
    number.high = 0;
    number.low = value;
    number.negative = 0;
    fvm_sinteger_write(&number, out, FVM_SRECORD_INTEGER_MAX, &used);
    return used;
}


/* Restrict schema identifiers, not application text, to portable printable non-space ASCII. */
static int binding_valid(const FVM_SBinding *binding)
{
    unsigned int i;
    if (!binding->tag || binding->tag > FVM_SRECORD_U32 || !binding->version || binding->version > FVM_SRECORD_U32 ||
        !binding->type_size || binding->type_size > FVM_SBINDINGS_TYPE_MAX)
    {
        return 0;
    }
    for (i = 0; i < binding->type_size; ++i)
    {
        if ((unsigned char)binding->type[i] < 0x21U || (unsigned char)binding->type[i] > 0x7eU)
        {
            return 0;
        }
    }
    return 1;
}


/* Registry descriptors are native policies, not wire records. */
static int table_valid(const FVM_SBindings *table)
{
    return table && table->struct_size == sizeof(*table) && table->capacity <= FVM_SBINDINGS_MAX &&
           table->count <= table->capacity && (table->slots || !table->capacity);
}


/* Linear lookup is bounded by MAX and is independent of VM/GC identity maps. */
static size_t find_tag(const FVM_SBindings *table, unsigned long tag)
{
    size_t i;
    for (i = 0; i < table->count; ++i)
    {
        if (table->slots[i].tag == tag)
        {
            return i;
        }
    }
    return table->count;
}


/* Parse a single association into native private scratch, never touching the registry. */
static int binding_read(const unsigned char **cursor, size_t *left, FVM_SBinding *binding)
{
    unsigned long size;
    memset(binding, 0, sizeof(*binding));
    if (field_read(cursor, left, &binding->tag) != FVM_S_OK ||
        field_read(cursor, left, &binding->version) != FVM_S_OK || field_read(cursor, left, &size) != FVM_S_OK ||
        size > FVM_SBINDINGS_TYPE_MAX || size > *left)
    {
        return 0;
    }
    binding->type_size = (unsigned int)size;
    memcpy(binding->type, *cursor, (size_t)size);
    *cursor += size;
    *left -= size;
    return binding_valid(binding);
}


/* Compare complete semantic association, ignoring native padding and inactive trailing bytes. */
static int same_binding(const FVM_SBinding *a, const FVM_SBinding *b)
{
    return a->tag == b->tag && a->version == b->version && a->type_size == b->type_size &&
           !memcmp(a->type, b->type, a->type_size);
}


/* Empty initialization preserves caller slot bytes, including on rejected arguments. */
FVM_SResult fvm_sbindings_init(FVM_SBindings *table, FVM_SBinding *slots, size_t capacity)
{
    if (!table || (!slots && capacity) || capacity > FVM_SBINDINGS_MAX)
    {
        return FVM_S_INVALID;
    }
    table->struct_size = sizeof(*table);
    table->slots = slots;
    table->capacity = capacity;
    table->count = 0;
    return FVM_S_OK;
}


/* Return a copied stable descriptor; nothing is borrowed from incoming record storage. */
FVM_SResult fvm_sbindings_lookup(const FVM_SBindings *table, unsigned long tag, FVM_SBinding *out)
{
    size_t index;
    if (!table_valid(table) || !out || tag > FVM_SRECORD_U32)
    {
        return FVM_S_INVALID;
    }
    index = find_tag(table, tag);
    if (!tag || index == table->count)
    {
        return FVM_S_UNKNOWN;
    }
    *out = table->slots[index];
    return FVM_S_OK;
}


/* Preflight with a small field scratch rather than a record-sized stack or hidden allocation. */
FVM_SResult
fvm_sbindings_payload_write(const FVM_SBinding *bindings, size_t count, void *output, size_t capacity, size_t *used)
{
    unsigned char scratch[FVM_SRECORD_INTEGER_MAX];
    unsigned char *out = (unsigned char *)output;
    size_t total, i, j, offset;
    if ((!bindings && count) || !out || !used || count > FVM_SBINDINGS_MAX)
    {
        return FVM_S_INVALID;
    }
    total = field_write(FVM_SBINDINGS_VERSION, scratch) + field_write((unsigned long)count, scratch);
    for (i = 0; i < count; ++i)
    {
        if (!binding_valid(bindings + i))
        {
            return FVM_S_INVALID;
        }
        for (j = 0; j < i; ++j)
        {
            if (bindings[j].tag == bindings[i].tag)
            {
                return FVM_S_INVALID;
            }
        }
        total += field_write(bindings[i].tag, scratch) + field_write(bindings[i].version, scratch) +
                 field_write(bindings[i].type_size, scratch) + bindings[i].type_size;
    }
    if (total > capacity)
    {
        return FVM_S_LIMIT;
    }
    offset = field_write(FVM_SBINDINGS_VERSION, out);
    offset += field_write((unsigned long)count, out + offset);
    for (i = 0; i < count; ++i)
    {
        offset += field_write(bindings[i].tag, out + offset);
        offset += field_write(bindings[i].version, out + offset);
        offset += field_write(bindings[i].type_size, out + offset);
        memcpy(out + offset, bindings[i].type, bindings[i].type_size);
        offset += bindings[i].type_size;
    }
    *used = offset;
    return FVM_S_OK;
}


/* Two passes guarantee atomic acceptance without a table-sized temporary allocation. */
FVM_SResult fvm_sbindings_apply(FVM_SBindings *table, const void *input, size_t size, size_t *used)
{
    FVM_SRecord record;
    FVM_SResult result;
    FVM_SBinding binding, previous;
    const unsigned char *cursor, *start, *prior;
    unsigned long version, count;
    size_t consumed, left, start_left, prior_left, i, j, added = 0, index;
    if (!table_valid(table) || !used)
    {
        return FVM_S_INVALID;
    }
    result = fvm_srecord_read(input, size, FVM_SRECORD_BODY_MAX, &record, &consumed);
    if (result != FVM_S_OK)
    {
        return result;
    }
    if (record.tag != FVM_SRECORD_CONTROL_TAG)
    {
        return FVM_S_INVALID;
    }
    cursor = record.payload;
    left = record.size;
    if (field_read(&cursor, &left, &version) != FVM_S_OK || version != FVM_SBINDINGS_VERSION ||
        field_read(&cursor, &left, &count) != FVM_S_OK || count > FVM_SBINDINGS_MAX)
    {
        return FVM_S_INVALID;
    }
    start = cursor;
    start_left = left;
    for (i = 0; i < count; ++i)
    {
        if (!binding_read(&cursor, &left, &binding))
        {
            return FVM_S_INVALID;
        }
        prior = start;
        prior_left = start_left;
        for (j = 0; j < i; ++j)
        {
            if (!binding_read(&prior, &prior_left, &previous) || previous.tag == binding.tag)
            {
                return FVM_S_INVALID;
            }
        }
        index = find_tag(table, binding.tag);
        if (index < table->count)
        {
            if (!same_binding(table->slots + index, &binding))
            {
                return FVM_S_CONFLICT;
            }
        }
        else
        {
            ++added;
        }
    }
    if (left)
    {
        return FVM_S_INVALID;
    }
    if (added > table->capacity - table->count)
    {
        return FVM_S_LIMIT;
    }
    cursor = start;
    left = start_left;
    for (i = 0; i < count; ++i)
    {
        binding_read(&cursor, &left, &binding);
        if (find_tag(table, binding.tag) == table->count)
        {
            table->slots[table->count++] = binding;
        }
    }
    *used = consumed;
    return FVM_S_OK;
}
