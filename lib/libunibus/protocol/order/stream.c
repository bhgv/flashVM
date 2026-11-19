/* Copyright (c) 2026 bhgv
 * EN: Incremental CRC-5 prefix plus original S-record decoder, one packet per successful push.
 * RU: Приём префикса CRC-5 и исходной S-записи по частям; одна запись на успешный push.
 */
#include "flashvm/sorder.h"
#include <string.h>

/* EN/RU: Validate initialized private descriptor before touching caller storage.
 * Проверить initialized private descriptor до обращения к caller storage.
 */


/* [standalone-contract:valid]
 * EN: valid: independent per-tag stream state.
 * RU: valid: независимое состояние тегов stream.
 * stream (const FVM_SOrderedStream *stream): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int valid(const FVM_SOrderedStream *stream)
{
    return stream && stream->struct_size == sizeof(*stream) && stream->filled <= FVM_SORDER_PREFIX_MAX &&
           stream->expected <= FVM_SORDER_PREFIX_MAX && stream->filled <= stream->expected &&
           stream->stream.struct_size == sizeof(FVM_SStream);
}

/* EN/RU: See sorder.h; temporary descriptor keeps invalid initialization atomic.
 * См. sorder.h; temporary descriptor сохраняет invalid initialization атомарной.
 */


/* [standalone-contract:fvm_sordered_stream_init]
 * EN: fvm_sordered_stream_init: independent per-tag stream state.
 * RU: fvm_sordered_stream_init: независимое состояние тегов stream.
 * stream (FVM_SOrderedStream *stream): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * storage (void *storage): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * capacity (size_t capacity): Actual caller-owned output capacity; it is not permission to write past the buffer.
 * RU: Фактическая ёмкость буфера вызывающего; выход за неё недопустим.
 * limit (size_t limit): Explicit local quota; permitted ranges and wire/body units are given in the description.
 * RU: Явная локальная квота; диапазон и единицы wire/body указаны в описании.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_sordered_stream_init(FVM_SOrderedStream *stream, void *storage, size_t capacity, size_t limit)
{
    FVM_SOrderedStream next; /* Private complete initialized descriptor. */
    FVM_SResult result;      /* Original decoder validation result. */
    if (!stream)
    {
        return FVM_S_INVALID;
    }
    memset(&next, 0, sizeof(next));
    result = fvm_sstream_init(&next.stream, storage, capacity, limit);
    if (result == FVM_S_OK)
    {
        next.struct_size = sizeof(next);
        *stream = next;
    }
    return result;
}

/* EN/RU: See sorder.h; input/view are disjoint from retained prefix and original decoder storage.
 * См. sorder.h; input/view disjoint от prefix и original decoder storage.
 */


/* [standalone-contract:fvm_sordered_stream_push]
 * EN: fvm_sordered_stream_push: independent per-tag stream state.
 * RU: fvm_sordered_stream_push: независимое состояние тегов stream.
 * stream (FVM_SOrderedStream *stream): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * input (const void *input): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * view (FVM_SOrderedRecord *view): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * used (size_t *used): Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below.
 * RU: Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_MORE. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_MORE. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_sordered_stream_push(
    FVM_SOrderedStream *stream, const void *input, size_t size, FVM_SOrderedRecord *view, size_t *used)
{
    const unsigned char *bytes = (const unsigned char *)input; /* Borrowed input fragments. */
    size_t consumed = 0;                                       /* Exact consumed bytes on this valid call. */
    size_t prefix;                                             /* Validated exact prefix extent. */
    size_t part = 0;                                           /* Bytes consumed by original S-stream decoder. */
    FVM_SRecord record;                                        /* Private borrowed original S-record view. */
    FVM_SResult result;                                        /* Latched result except MORE. */
    if (!valid(stream) || (!input && size) || !view || !used)
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
    if (!stream->expected && size)
    {
        stream->prefix[0] = bytes[consumed++];
        stream->filled = 1;
        stream->expected = (stream->prefix[0] & 7U) + 2U;
    }
    while (consumed < size && stream->filled < stream->expected)
    {
        stream->prefix[stream->filled++] = bytes[consumed++];
    }
    *used = consumed;
    if (!stream->expected || stream->filled < stream->expected)
    {
        return FVM_S_MORE;
    }
    result = fvm_sordinal_read(stream->prefix, stream->filled, &stream->ordinal, &prefix);
    if (result != FVM_S_OK)
    {
        stream->error = result;
        return result;
    }
    result =
        fvm_sstream_push(&stream->stream, consumed < size ? bytes + consumed : NULL, size - consumed, &record, &part);
    *used += part;
    if (result == FVM_S_OK)
    {
        FVM_SOrderedRecord next; /* Publish only the complete combined checked view. */
        next.ordinal = stream->ordinal;
        next.record = record;
        next.encoded_size = prefix + record.encoded_size;
        stream->ready = 1;
        *view = next;
    }
    else if (result != FVM_S_MORE)
    {
        stream->error = result;
    }
    return result;
}

/* EN/RU: See sorder.h; reset only at an explicitly known packet boundary, never guess resynchronization.
 * См. sorder.h; reset лишь при известной packet boundary, без угадывания resynchronization.
 */


/* [standalone-contract:fvm_sordered_stream_reset]
 * EN: fvm_sordered_stream_reset: independent per-tag stream state.
 * RU: fvm_sordered_stream_reset: независимое состояние тегов stream.
 * stream (FVM_SOrderedStream *stream): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_SResult fvm_sordered_stream_reset(FVM_SOrderedStream *stream)
{
    FVM_SResult result; /* Original decoder reset status. */
    if (!valid(stream))
    {
        return FVM_S_INVALID;
    }
    result = fvm_sstream_reset(&stream->stream);
    if (result == FVM_S_OK)
    {
        stream->filled = 0;
        stream->expected = 0;
        stream->ready = 0;
        stream->error = FVM_S_OK;
    }
    return result;
}
