/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: incremental checked S-record input and caller-buffer lifetime.
 * RU: инкрементальный проверяемый ввод S-записей и время жизни буфера.
 */
/* Incremental framing copies bounded fragments and publishes only fully verified S-records. */
#include "flashvm/sstream.h"
#include <string.h>

/* Native descriptor validation; callers never mutate the decoder fields directly. */


/* [standalone-contract:stream_valid]
 * EN: stream_valid: incremental checked S-record input and caller-buffer lifetime.
 * RU: stream_valid: инкрементальный проверяемый ввод S-записей и время жизни буфера.
 * stream (const FVM_SStream *stream): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int stream_valid(const FVM_SStream *stream)
{
    return stream && stream->struct_size == sizeof(*stream) && stream->buffer &&
           stream->capacity >= FVM_SSTREAM_MIN_CAPACITY && stream->filled <= stream->capacity &&
           stream->expected <= stream->capacity && (!stream->expected || stream->filled <= stream->expected) &&
           stream->limit && stream->limit <= FVM_SRECORD_BODY_MAX;
}

/* Check metadata using the same canonical unsigned 32-bit wire integers as the complete codec. */


/* [standalone-contract:header_field]
 * EN: header_field: incremental checked S-record input and caller-buffer lifetime.
 * RU: header_field: инкрементальный проверяемый ввод S-записей и время жизни буфера.
 * input (const unsigned char *input): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
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
static FVM_SResult header_field(const unsigned char *input, size_t size, unsigned long *out, size_t *used)
{
    FVM_SInteger value;
    FVM_SResult result = fvm_sinteger_read(input, size, &value, used);
    if (result != FVM_S_OK)
    {
        return result;
    }
    if (value.high || value.negative)
    {
        return FVM_S_INVALID;
    }
    *out = value.low;
    return FVM_S_OK;
}

/* Initialize an empty prefix; no byte storage is cleared or allocated. */


/* [standalone-contract:fvm_sstream_init]
 * EN: fvm_sstream_init: incremental checked S-record input and caller-buffer lifetime.
 * RU: fvm_sstream_init: инкрементальный проверяемый ввод S-записей и время жизни буфера.
 * stream (FVM_SStream *stream): Writable decoder state for one receive direction; initialize before push.
 * RU: Изменяемое состояние одного направления приёма; init до push.
 * buffer (void *buffer): Caller-owned receive storage, disjoint from stream and later input fragments; retain through decoder use.
 * RU: Память приёма вызывающего, отдельно от stream и входных фрагментов; сохраняется на время работы decoder.
 * capacity (size_t capacity): Accessible bytes in buffer; may impose a tighter bound than limit.
 * RU: Доступные байты buffer; могут ограничивать запись сильнее limit.
 * limit (size_t limit): Maximum body bytes, including encoded tag: 1..FVM_SRECORD_BODY_MAX.
 * RU: Максимум байтов body, включая encoded tag: 1..FVM_SRECORD_BODY_MAX.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_sstream_init(FVM_SStream *stream, void *buffer, size_t capacity, size_t limit)
{
    if (!stream || !buffer || capacity < FVM_SSTREAM_MIN_CAPACITY || !limit || limit > FVM_SRECORD_BODY_MAX)
    {
        return FVM_S_INVALID;
    }
    stream->struct_size = sizeof(*stream);
    stream->buffer = (unsigned char *)buffer;
    stream->capacity = capacity;
    stream->limit = limit;
    stream->filled = stream->expected = 0;
    stream->ready = 0;
    stream->error = FVM_S_OK;
    return FVM_S_OK;
}

/* Explicit discard is separate from input consumption and remote cancellation. */


/* [standalone-contract:fvm_sstream_reset]
 * EN: fvm_sstream_reset: incremental checked S-record input and caller-buffer lifetime.
 * RU: fvm_sstream_reset: инкрементальный проверяемый ввод S-записей и время жизни буфера.
 * stream (FVM_SStream *stream): Initialized decoder to reuse; invalidates the previous borrowed view and clears latched errors.
 * RU: Decoder для повторного использования; делает прежний view недействительным и сбрасывает latched errors.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_sstream_reset(FVM_SStream *stream)
{
    if (!stream_valid(stream))
    {
        return FVM_S_INVALID;
    }
    stream->filled = stream->expected = 0;
    stream->ready = 0;
    stream->error = FVM_S_OK;
    return FVM_S_OK;
}

/* Bound prefix work to integer headers; after framing, each payload fragment is copied once. */


/* [standalone-contract:fvm_sstream_push]
 * EN: fvm_sstream_push: incremental checked S-record input and caller-buffer lifetime.
 * RU: fvm_sstream_push: инкрементальный проверяемый ввод S-записей и время жизни буфера.
 * stream (FVM_SStream *stream): Initialized decoder; a ready view requires reset before another push.
 * RU: Инициализированный decoder; после готового view нужен reset до нового push.
 * input (const void *input): Borrowed new fragment, disjoint from receive buffer/state; NULL only with size zero.
 * RU: Новый заимствованный фрагмент, отдельно от receive buffer/state; NULL только при size=0.
 * size (size_t size): Accessible bytes in this new fragment, not total bytes retained by the decoder.
 * RU: Доступные байты нового фрагмента, не общий накопленный размер.
 * view (FVM_SRecord *view): Published only on OK; borrows receive storage until reset/disposal.
 * RU: Публикуется только при OK; заимствует receive storage до reset/disposal.
 * used (size_t *used): Consumed fragment bytes, including on MORE/data error; retain only the unconsumed suffix. Invalid arguments leave it unchanged.
 * RU: Поглощённые байты фрагмента, также при MORE/data error; сохранять только непоглощённый хвост. Неверные аргументы оставляют used прежним.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_sstream_push(FVM_SStream *stream, const void *input, size_t size, FVM_SRecord *view, size_t *used)
{
    const unsigned char *bytes = (const unsigned char *)input;
    FVM_SResult result = FVM_S_MORE;
    unsigned long checksum, length;
    size_t consumed = 0, cs, ls, copy, verified;
    if (!stream_valid(stream) || (!input && size) || !view || !used)
    {
        return FVM_S_INVALID;
    }
    *used = 0;
    if (stream->ready)
    {
        return FVM_S_INVALID;
    }
    if (stream->error != FVM_S_OK)
    {
        return stream->error;
    }
    while (consumed < size)
    {
        if (!stream->expected)
        {
            if (stream->filled == stream->capacity)
            {
                result = FVM_S_LIMIT;
                break;
            }
            stream->buffer[stream->filled++] = bytes[consumed++];
            result = header_field(stream->buffer, stream->filled, &checksum, &cs);
            if (result == FVM_S_MORE)
            {
                continue;
            }
            if (result != FVM_S_OK)
            {
                break;
            }
            result = header_field(stream->buffer + cs, stream->filled - cs, &length, &ls);
            if (result == FVM_S_MORE)
            {
                continue;
            }
            if (result != FVM_S_OK)
            {
                break;
            }
            if (!length)
            {
                result = FVM_S_INVALID;
                break;
            }
            if (length > stream->limit || length > stream->capacity - stream->filled)
            {
                result = FVM_S_LIMIT;
                break;
            }
            stream->expected = cs + ls + (size_t)length;
        }
        else
        {
            copy = stream->expected - stream->filled;
            if (copy > size - consumed)
            {
                copy = size - consumed;
            }
            memcpy(stream->buffer + stream->filled, bytes + consumed, copy);
            stream->filled += copy;
            consumed += copy;
            if (stream->filled == stream->expected)
            {
                result = fvm_srecord_read(stream->buffer, stream->filled, stream->limit, view, &verified);
                if (result == FVM_S_OK)
                {
                    stream->ready = 1;
                }
                else if (result == FVM_S_MORE)
                {
                    result = FVM_S_INVALID;
                }
                break;
            }
        }
        result = FVM_S_MORE;
    }
    *used = consumed;
    if (result != FVM_S_OK && result != FVM_S_MORE)
    {
        stream->error = result;
    }
    return result;
}
