/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: direction-local numeric tag to schema identity bindings.
 * RU: привязки числовых тегов направления к идентичности схемы.
 */
/* Bounded directional metadata without allocation, native object addresses or global tag tables. */
#include "flashvm/sbindings.h"
#include <string.h>

/* Read a canonical unsigned 32-bit metadata field and advance a private cursor only on success. */


/* [standalone-contract:field_read]
 * EN: field_read: direction-local numeric tag to schema identity bindings.
 * RU: field_read: привязки числовых тегов направления к идентичности схемы.
 * cursor (const unsigned char **cursor): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * left (size_t *left): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * out (unsigned long *out): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
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


/* [standalone-contract:field_write]
 * EN: field_write: direction-local numeric tag to schema identity bindings.
 * RU: field_write: привязки числовых тегов направления к идентичности схемы.
 * value (unsigned long value): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * out (unsigned char *out): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * Return: size_t; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: size_t; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
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


/* [standalone-contract:binding_valid]
 * EN: binding_valid: direction-local numeric tag to schema identity bindings.
 * RU: binding_valid: привязки числовых тегов направления к идентичности схемы.
 * binding (const FVM_SBinding *binding): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
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


/* [standalone-contract:table_valid]
 * EN: table_valid: direction-local numeric tag to schema identity bindings.
 * RU: table_valid: привязки числовых тегов направления к идентичности схемы.
 * table (const FVM_SBindings *table): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int table_valid(const FVM_SBindings *table)
{
    return table && table->struct_size == sizeof(*table) && table->capacity <= FVM_SBINDINGS_MAX &&
           table->count <= table->capacity && (table->slots || !table->capacity);
}

/* Linear lookup is bounded by MAX and is independent of VM/GC identity maps. */


/* [standalone-contract:find_tag]
 * EN: find_tag: direction-local numeric tag to schema identity bindings.
 * RU: find_tag: привязки числовых тегов направления к идентичности схемы.
 * table (const FVM_SBindings *table): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * Return: size_t; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: size_t; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
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


/* [standalone-contract:binding_read]
 * EN: binding_read: direction-local numeric tag to schema identity bindings.
 * RU: binding_read: привязки числовых тегов направления к идентичности схемы.
 * cursor (const unsigned char **cursor): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * left (size_t *left): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * binding (FVM_SBinding *binding): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
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


/* [standalone-contract:same_binding]
 * EN: same_binding: direction-local numeric tag to schema identity bindings.
 * RU: same_binding: привязки числовых тегов направления к идентичности схемы.
 * a (const FVM_SBinding *a): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * b (const FVM_SBinding *b): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int same_binding(const FVM_SBinding *a, const FVM_SBinding *b)
{
    return a->tag == b->tag && a->version == b->version && a->type_size == b->type_size &&
           !memcmp(a->type, b->type, a->type_size);
}

/* Empty initialization preserves caller slot bytes, including on rejected arguments. */


/* [standalone-contract:fvm_sbindings_init]
 * EN: fvm_sbindings_init: direction-local numeric tag to schema identity bindings.
 * RU: fvm_sbindings_init: привязки числовых тегов направления к идентичности схемы.
 * table (FVM_SBindings *table): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * slots (FVM_SBinding *slots): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * capacity (size_t capacity): Actual caller-owned output capacity; it is not permission to write past the buffer.
 * RU: Фактическая ёмкость буфера вызывающего; выход за неё недопустим.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
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


/* [standalone-contract:fvm_sbindings_lookup]
 * EN: fvm_sbindings_lookup: direction-local numeric tag to schema identity bindings.
 * RU: fvm_sbindings_lookup: привязки числовых тегов направления к идентичности схемы.
 * table (const FVM_SBindings *table): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * out (FVM_SBinding *out): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_OK, FVM_S_UNKNOWN. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_OK, FVM_S_UNKNOWN. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
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


/* [standalone-contract:fvm_sbindings_payload_write]
 * EN: fvm_sbindings_payload_write: direction-local numeric tag to schema identity bindings.
 * RU: fvm_sbindings_payload_write: привязки числовых тегов направления к идентичности схемы.
 * bindings (const FVM_SBinding *bindings): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * count (size_t count): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * output (void *output): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * capacity (size_t capacity): Actual caller-owned output capacity; it is not permission to write past the buffer.
 * RU: Фактическая ёмкость буфера вызывающего; выход за неё недопустим.
 * used (size_t *used): Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below.
 * RU: Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
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


/* [standalone-contract:fvm_sbindings_apply]
 * EN: fvm_sbindings_apply: direction-local numeric tag to schema identity bindings.
 * RU: fvm_sbindings_apply: привязки числовых тегов направления к идентичности схемы.
 * table (FVM_SBindings *table): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * input (const void *input): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * used (size_t *used): Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below.
 * RU: Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_CONFLICT, FVM_S_INVALID, FVM_S_LIMIT, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
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
