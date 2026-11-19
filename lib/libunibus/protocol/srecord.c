/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: canonical integers, bounded S-record framing and checksum validation.
 * RU: канонические целые, framing S-записей и проверка checksum.
 */
/* Standalone C89 binary S-record codec; all bytes are explicit, no VM or native layout on wire. */
#include "flashvm/srecord.h"
#include <string.h>

/* Compute canonical integer width while dividing a portable magnitude by 128. */


/* [standalone-contract:integer_size]
 * EN: integer_size: canonical integers, bounded S-record framing and checksum validation.
 * RU: integer_size: канонические целые, framing S-записей и проверка checksum.
 * value (FVM_SInteger value): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: size_t; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: size_t; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static size_t integer_size(FVM_SInteger value)
{
    size_t count = 1;
    while (value.high || value.low > FVM_SRECORD_FIRST_MASK)
    {
        value.low = ((value.low >> 7) | (value.high << 25)) & FVM_SRECORD_U32;
        value.high >>= 7;
        ++count;
    }
    return count;
}

/* Encode in temporary storage first, making insufficient-capacity rejection atomic. */


/* [standalone-contract:fvm_sinteger_write]
 * EN: fvm_sinteger_write: canonical integers, bounded S-record framing and checksum validation.
 * RU: fvm_sinteger_write: канонические целые, framing S-записей и проверка checksum.
 * value (const FVM_SInteger *value): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * out (unsigned char *out): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
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
FVM_SResult fvm_sinteger_write(const FVM_SInteger *value, unsigned char *out, size_t capacity, size_t *used)
{
    unsigned char bytes[FVM_SRECORD_INTEGER_MAX];
    FVM_SInteger part;
    size_t count, i;
    if (!value || !out || !used || value->high > FVM_SRECORD_U32 || value->low > FVM_SRECORD_U32 ||
        (value->negative != 0 && value->negative != 1) || (value->negative && !value->high && !value->low))
    {
        return FVM_S_INVALID;
    }
    count = integer_size(*value);
    if (capacity < count)
    {
        return FVM_S_LIMIT;
    }
    part = *value;
    for (i = count; i > 1; --i)
    {
        bytes[i - 1] = (unsigned char)(part.low & FVM_SRECORD_NEXT_MASK);
        part.low = ((part.low >> 7) | (part.high << 25)) & FVM_SRECORD_U32;
        part.high >>= 7;
    }
    bytes[0] = (unsigned char)(part.low | (value->negative ? FVM_SRECORD_SIGN : 0));
    bytes[count - 1] |= FVM_SRECORD_FINAL;
    memcpy(out, bytes, count);
    *used = count;
    return FVM_S_OK;
}

/* Reject overflow, redundant groups and negative zero without publishing partial values. */


/* [standalone-contract:fvm_sinteger_read]
 * EN: fvm_sinteger_read: canonical integers, bounded S-record framing and checksum validation.
 * RU: fvm_sinteger_read: канонические целые, framing S-записей и проверка checksum.
 * in (const unsigned char *in): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * value (FVM_SInteger *value): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * used (size_t *used): Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below.
 * RU: Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_MORE, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_MORE, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_sinteger_read(const unsigned char *in, size_t size, FVM_SInteger *value, size_t *used)
{
    FVM_SInteger result;
    size_t i;
    unsigned int byte;
    if ((!in && size) || !value || !used)
    {
        return FVM_S_INVALID;
    }
    if (!size)
    {
        return FVM_S_MORE;
    }
    result.high = 0;
    result.low = in[0] & FVM_SRECORD_FIRST_MASK;
    result.negative = (in[0] & FVM_SRECORD_SIGN) != 0;
    for (i = 0; i < size && i < FVM_SRECORD_INTEGER_MAX; ++i)
    {
        byte = in[i];
        if (i)
        {
            if (result.high > FVM_SRECORD_SHIFT_LIMIT)
            {
                return FVM_S_INVALID;
            }
            result.high = ((result.high << 7) | (result.low >> 25)) & FVM_SRECORD_U32;
            result.low = ((result.low << 7) | (byte & FVM_SRECORD_NEXT_MASK)) & FVM_SRECORD_U32;
        }
        if (byte & FVM_SRECORD_FINAL)
        {
            if ((result.negative && !result.high && !result.low) || integer_size(result) != i + 1)
            {
                return FVM_S_INVALID;
            }
            *value = result;
            *used = i + 1;
            return FVM_S_OK;
        }
    }
    return i == FVM_SRECORD_INTEGER_MAX ? FVM_S_INVALID : FVM_S_MORE;
}

/* Unsigned 32-bit field wrapper, sharing the exact signed integer wire rules. */


/* [standalone-contract:write_field]
 * EN: write_field: canonical integers, bounded S-record framing and checksum validation.
 * RU: write_field: канонические целые, framing S-записей и проверка checksum.
 * value (unsigned long value): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * out (unsigned char *out): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * used (size_t *used): Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below.
 * RU: Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже.
 * Return: FVM_SResult; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static FVM_SResult write_field(unsigned long value, unsigned char *out, size_t *used)
{
    FVM_SInteger number;
    number.high = 0;
    number.low = value;
    number.negative = 0;
    return fvm_sinteger_write(&number, out, FVM_SRECORD_INTEGER_MAX, used);
}

/* Decode bounded nonnegative framing metadata; negative or wider values are not lengths/tags. */


/* [standalone-contract:read_field]
 * EN: read_field: canonical integers, bounded S-record framing and checksum validation.
 * RU: read_field: канонические целые, framing S-записей и проверка checksum.
 * in (const unsigned char *in): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * out (unsigned long *out): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * used (size_t *used): Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below.
 * RU: Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static FVM_SResult read_field(const unsigned char *in, size_t size, unsigned long *out, size_t *used)
{
    FVM_SInteger number;
    FVM_SResult result = fvm_sinteger_read(in, size, &number, used);
    if (result != FVM_S_OK)
    {
        return result;
    }
    if (number.negative || number.high)
    {
        return FVM_S_INVALID;
    }
    *out = number.low;
    return FVM_S_OK;
}

/* CRC uses octets and masked 32-bit arithmetic equally on ILP32, LLP64 and LP64. */


/* [standalone-contract:crc_bytes]
 * EN: crc_bytes: canonical integers, bounded S-record framing and checksum validation.
 * RU: crc_bytes: канонические целые, framing S-записей и проверка checksum.
 * crc (unsigned long crc): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * in (const unsigned char *in): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * Return: unsigned long; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: unsigned long; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static unsigned long crc_bytes(unsigned long crc, const unsigned char *in, size_t size)
{
    unsigned int bit;
    size_t i;
    for (i = 0; i < size; ++i)
    {
        crc ^= in[i];
        for (bit = 0; bit < 8; ++bit)
        {
            crc = (crc >> 1) ^ ((crc & 1UL) ? FVM_SRECORD_CRC_POLY : 0UL);
        }
    }
    return crc & FVM_SRECORD_U32;
}

/* Retain a short checksum for short bodies; the full CRC starts at five length groups. */


/* [standalone-contract:crc_mask]
 * EN: crc_mask: canonical integers, bounded S-record framing and checksum validation.
 * RU: crc_mask: канонические целые, framing S-записей и проверка checksum.
 * length_size (size_t length_size): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: unsigned long; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: unsigned long; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static unsigned long crc_mask(size_t length_size)
{
    unsigned int bits = 6U + 7U * (unsigned int)(length_size - 1);
    return bits >= 32U ? FVM_SRECORD_U32 : (1UL << bits) - 1UL;
}

/* Build complete framing after all validation, with no dynamic allocation or hidden state. */


/* [standalone-contract:fvm_srecord_write_pair]
 * EN: fvm_srecord_write_pair: canonical integers, bounded S-record framing and checksum validation.
 * RU: fvm_srecord_write_pair: канонические целые, framing S-записей и проверка checksum.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * first (const void *first): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * first_size (size_t first_size): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * second (const void *second): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * second_size (size_t second_size): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * out (unsigned char *out): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
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
FVM_SResult fvm_srecord_write_pair(unsigned long tag,
                                   const void *first,
                                   size_t first_size,
                                   const void *second,
                                   size_t second_size,
                                   unsigned char *out,
                                   size_t capacity,
                                   size_t *used)
{
    unsigned char tag_bytes[FVM_SRECORD_INTEGER_MAX], length_bytes[FVM_SRECORD_INTEGER_MAX];
    unsigned char checksum[FVM_SRECORD_INTEGER_MAX];
    size_t ts, ls, cs, body, total;
    unsigned long crc;
    if (!out || !used || (!first && first_size) || (!second && second_size) || tag > FVM_SRECORD_U32)
    {
        return FVM_S_INVALID;
    }
    write_field(tag, tag_bytes, &ts);
    if (first_size > FVM_SRECORD_BODY_MAX - ts || second_size > FVM_SRECORD_BODY_MAX - ts - first_size)
    {
        return FVM_S_LIMIT;
    }
    if (first_size > (size_t)-1 - ts || second_size > (size_t)-1 - ts - first_size)
    {
        return FVM_S_LIMIT;
    }
    body = ts + first_size + second_size;
    write_field((unsigned long)body, length_bytes, &ls);
    crc = crc_bytes(FVM_SRECORD_U32, length_bytes, ls);
    crc = crc_bytes(crc, tag_bytes, ts);
    crc = crc_bytes(crc, (const unsigned char *)first, first_size);
    crc = (crc_bytes(crc, (const unsigned char *)second, second_size) ^ FVM_SRECORD_U32) & crc_mask(ls);
    write_field(crc, checksum, &cs);
    if (body > (size_t)-1 - cs - ls)
    {
        return FVM_S_LIMIT;
    }
    total = cs + ls + body;
    if (capacity < total)
    {
        return FVM_S_LIMIT;
    }
    memcpy(out, checksum, cs);
    memcpy(out + cs, length_bytes, ls);
    memcpy(out + cs + ls, tag_bytes, ts);
    if (first_size)
    {
        memcpy(out + cs + ls + ts, first, first_size);
    }
    if (second_size)
    {
        memcpy(out + cs + ls + ts + first_size, second, second_size);
    }
    *used = total;
    return FVM_S_OK;
}

/* Preserve the ordinary single-span writer through the same atomic framing implementation. */


/* [standalone-contract:fvm_srecord_write]
 * EN: fvm_srecord_write: canonical integers, bounded S-record framing and checksum validation.
 * RU: fvm_srecord_write: канонические целые, framing S-записей и проверка checksum.
 * tag (unsigned long tag): Unsigned direction-local tag, 0..0xffffffff; transport tag 0 is service-owned.
 * RU: Беззнаковый тег направления 0..0xffffffff; transport tag 0 принадлежит обслуживанию.
 * payload (const void *payload): Borrowed opaque bytes, possibly nested records; disjoint from output/state.
 * RU: Заимствованные непрозрачные байты, возможно вложенные записи; отдельно от output/state.
 * size (size_t size): Payload bytes; tag+payload must fit the body profile bound.
 * RU: Байты payload; tag+payload должны помещаться в предел body-профиля.
 * out (unsigned char *out): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * capacity (size_t capacity): Accessible destination bytes; insufficient space reports LIMIT.
 * RU: Доступные выходные байты; нехватка места даёт LIMIT.
 * used (size_t *used): Complete encoded byte count on OK; unchanged on failure.
 * RU: Полная кодированная длина при OK; при отказе не меняется.
 * Return: FVM_SResult; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_srecord_write(
    unsigned long tag, const void *payload, size_t size, unsigned char *out, size_t capacity, size_t *used)
{
    return fvm_srecord_write_pair(tag, payload, size, 0, 0, out, capacity, used);
}

/* Framing bounds precede CRC; tag and payload are exposed only after checksum acceptance. */


/* [standalone-contract:fvm_srecord_read]
 * EN: fvm_srecord_read: canonical integers, bounded S-record framing and checksum validation.
 * RU: fvm_srecord_read: канонические целые, framing S-записей и проверка checksum.
 * input (const void *input): Borrowed input bytes; preserve them throughout use of the returned record view.
 * RU: Заимствованный вход; сохраняется всё время использования record view.
 * size (size_t size): Accessible input bytes; trailing records may remain unconsumed.
 * RU: Доступные входные байты; последующие записи могут остаться непоглощёнными.
 * limit (size_t limit): Allowed body bytes, including encoded tag: 1..FVM_SRECORD_BODY_MAX.
 * RU: Допустимые body-байты, включая encoded tag: 1..FVM_SRECORD_BODY_MAX.
 * view (FVM_SRecord *view): Verified borrowed record, published only on OK.
 * RU: Проверенная заимствованная запись, публикуемая только при OK.
 * used (size_t *used): Bytes of exactly one record on OK; unchanged on any other result.
 * RU: Байты ровно одной записи при OK; иначе не меняется.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_CHECKSUM, FVM_S_INVALID, FVM_S_LIMIT, FVM_S_MORE, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_CHECKSUM, FVM_S_INVALID, FVM_S_LIMIT, FVM_S_MORE, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_srecord_read(const void *input, size_t size, size_t limit, FVM_SRecord *view, size_t *used)
{
    const unsigned char *in = (const unsigned char *)input;
    unsigned long checksum, length, tag, crc;
    size_t cs, ls, ts, remaining;
    FVM_SResult result;
    FVM_SRecord record;
    if ((!in && size) || !view || !used || !limit || limit > FVM_SRECORD_BODY_MAX)
    {
        return FVM_S_INVALID;
    }
    result = read_field(in, size, &checksum, &cs);
    if (result != FVM_S_OK)
    {
        return result;
    }
    result = read_field(in + cs, size - cs, &length, &ls);
    if (result != FVM_S_OK)
    {
        return result;
    }
    if (!length)
    {
        return FVM_S_INVALID;
    }
    if (length > limit)
    {
        return FVM_S_LIMIT;
    }
    remaining = size - cs - ls;
    if (length > remaining)
    {
        return FVM_S_MORE;
    }
    crc = (crc_bytes(FVM_SRECORD_U32, in + cs, ls + (size_t)length) ^ FVM_SRECORD_U32) & crc_mask(ls);
    if (crc != checksum)
    {
        return FVM_S_CHECKSUM;
    }
    result = read_field(in + cs + ls, (size_t)length, &tag, &ts);
    if (result != FVM_S_OK)
    {
        return FVM_S_INVALID;
    }
    record.tag = tag;
    record.payload = in + cs + ls + ts;
    record.size = (size_t)length - ts;
    record.encoded_size = cs + ls + (size_t)length;
    *view = record;
    *used = record.encoded_size;
    return FVM_S_OK;
}
