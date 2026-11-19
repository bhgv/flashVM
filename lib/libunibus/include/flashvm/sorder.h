/* Copyright (c) 2026 bhgv
 * EN: Compact unsigned64 sequence prefix with CRC-5, checked S-record packets and incremental reception.
 * RU: Компактный 64-битный префикс с CRC-5, проверка пакетов S-записей и приём по частям.
 */
#ifndef FLASHVM_SORDER_H
#define FLASHVM_SORDER_H
#include "sstream.h"
#define FVM_SORDER_PREFIX_MAX 9U /* One CRC/length byte plus one to eight unsigned big-endian bytes. */
#ifdef __cplusplus
extern "C"
{
#endif


/* EN/RU: Portable unsigned64 magnitude in two checked32-bit limbs; never serialize native struct layout.
 * Переносимое 64-битное число в двух проверяемых 32-битных частях; раскладка native-структуры в протокол не попадает.
 */
typedef struct FVM_SOrdinal
{
    unsigned long high; /* Most significant32 bits, <=U32 even on LP64. */
    unsigned long low;  /* Least significant32 bits, <=U32 even on LP64. */
} FVM_SOrdinal;

/* EN/RU: Complete verified packet, borrowed S-record body and copied ordinal, valid until input/storage changes.
 * Проверенный пакет: номер скопирован, данные заимствованы до изменения исходного буфера.
 */
typedef struct FVM_SOrderedRecord
{
    FVM_SOrdinal ordinal; /* Copied unsigned sequence, no generation field in data packets. */
    FVM_SRecord record;   /* Verified original nested-capable S-record view. */
    size_t encoded_size;  /* Prefix plus exactly one complete S-record. */
} FVM_SOrderedRecord;

/* EN/RU: Encode canonical prefix, S_OK/LIMIT/INVALID, outputs/used unchanged on failure; no heap, C89 ABI.
 * Header low3=number_bytes-1, high5=CRC over those3 bits followed by number bytes. MSB first, poly0x05,
 * init0x1f/xorout0x1f, no reflection or padding. Zero is one zero byte, leading zero bytes forbidden.
 * Каноническое кодирование префикса: младшие 3 бита — длина минус один, старшие 5 — CRC длины и номера; отказ не меняет результат.
 */


/* [standalone-contract:fvm_sordinal_write]
 * EN: fvm_sordinal_write: unsigned64 ordinal widths and CRC5 validation.
 * RU: fvm_sordinal_write: ширины unsigned64 номеров и проверка CRC5.
 * value (const FVM_SOrdinal *value): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
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
FVM_API FVM_SResult fvm_sordinal_write(const FVM_SOrdinal *value, void *output, size_t capacity, size_t *used);


/* EN/RU: Decode one canonical prefix, MORE until complete; outputs/used unchanged except on OK.
 * CRC is a corruption check, not authentication or a guarantee that prefix belongs to another valid body.
 * Прочитать префикс; MORE до завершения. Результат и used меняются лишь при OK; CRC не удостоверяет отправителя.
 */


/* [standalone-contract:fvm_sordinal_read]
 * EN: fvm_sordinal_read: unsigned64 ordinal widths and CRC5 validation.
 * RU: fvm_sordinal_read: ширины unsigned64 номеров и проверка CRC5.
 * input (const void *input): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * value (FVM_SOrdinal *value): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * used (size_t *used): Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below.
 * RU: Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_CHECKSUM, FVM_S_INVALID, FVM_S_MORE, FVM_S_OK. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_CHECKSUM, FVM_S_INVALID, FVM_S_MORE, FVM_S_OK. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_API FVM_SResult fvm_sordinal_read(const void *input, size_t size, FVM_SOrdinal *value, size_t *used);


/* EN/RU: Encode prefix plus one original S-record atomically, bounded by original global body limit.
 * Nonoverlapping borrowed inputs/output, no heap; output/used unchanged on any refusal.
 * Атомарно кодировать префикс и одну исходную S-запись; буферы не пересекаются, выделения памяти нет.
 */


/* [standalone-contract:fvm_sordered_write]
 * EN: fvm_sordered_write: checked ordinal prefixes around unchanged S-record bytes.
 * RU: fvm_sordered_write: проверяемые префиксы номеров вокруг неизменённых S-record байт.
 * ordinal (const FVM_SOrdinal *ordinal): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * tag (unsigned long tag): Identifier scoped to this direction/namespace; outer0 is reserved for service traffic.
 * RU: ID этого направления/пространства; внешний0 зарезервирован для служебного трафика.
 * bytes (const void *bytes): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * output (void *output): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * capacity (size_t capacity): Actual caller-owned output capacity; it is not permission to write past the buffer.
 * RU: Фактическая ёмкость буфера вызывающего; выход за неё недопустим.
 * used (size_t *used): Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below.
 * RU: Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID, FVM_S_LIMIT. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID, FVM_S_LIMIT. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_API FVM_SResult fvm_sordered_write(const FVM_SOrdinal *ordinal,
                                       unsigned long tag,
                                       const void *bytes,
                                       size_t size,
                                       void *output,
                                       size_t capacity,
                                       size_t *used);


/* EN/RU: Validate prefix and exactly one S-record, limit applies to S-record body; trailing packets remain unread.
 * Borrow input through view lifetime; view/used unchanged on MORE/error, original CRC still checked.
 * Проверить префикс и одну S-запись; следующие пакеты не потребляются, MORE/ошибка не меняют view/used.
 */


/* [standalone-contract:fvm_sordered_read]
 * EN: fvm_sordered_read: checked ordinal prefixes around unchanged S-record bytes.
 * RU: fvm_sordered_read: проверяемые префиксы номеров вокруг неизменённых S-record байт.
 * input (const void *input): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (size_t size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * limit (size_t limit): Explicit local quota; permitted ranges and wire/body units are given in the description.
 * RU: Явная локальная квота; диапазон и единицы wire/body указаны в описании.
 * view (FVM_SOrderedRecord *view): Caller-owned protocol value/state; native layout is never a wire representation.
 * RU: Значение/состояние протокола вызывающего; native layout не является wire-представлением.
 * used (size_t *used): Caller-owned storage for the exact consumed/produced extent; failure publication rules are documented below.
 * RU: Место вызывающего для точного принятого/выданного размера; правила публикации при отказе приведены ниже.
 * Return: FVM_SResult; direct result constants in this body: FVM_S_INVALID. Forwarded results preserve the called API's status.
 * Возврат: FVM_SResult; прямые константы результата тела: FVM_S_INVALID. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
FVM_API FVM_SResult
fvm_sordered_read(const void *input, size_t size, size_t limit, FVM_SOrderedRecord *view, size_t *used);

/* EN/RU: Caller-buffered decoder, private fields after init; no transport, VM, heap or implicit resynchronization.
 * Декодер на буфере caller; после init поля закрыты для записи. Без transport, VM, heap и поиска новой границы.
 */
typedef struct FVM_SOrderedStream
{
    size_t struct_size;                          /* Exact initialized native descriptor size. */
    unsigned char prefix[FVM_SORDER_PREFIX_MAX]; /* At most9 copied prefix bytes. */
    size_t filled;                               /* Prefix bytes retained. */
    size_t expected;                             /* Exact prefix extent after first byte, zero before it. */
    FVM_SOrdinal ordinal;                        /* Validated copied sequence once prefix complete. */
    FVM_SStream stream;                          /* Original S-record decoder over caller-owned disjoint storage. */
    int ready;                                   /* One verified borrowed view is outstanding. */
    FVM_SResult error;                           /* Latched framing/checksum error until explicit reset. */
} FVM_SOrderedStream;

/* EN/RU: Initialize decoder over borrowed storage, unchanged descriptor on refusal; same body bounds as SStream.
 * Инициализировать декодер на заимствованном storage с границами SStream; отказ не меняет descriptor.
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
FVM_API FVM_SResult fvm_sordered_stream_init(FVM_SOrderedStream *stream, void *storage, size_t capacity, size_t limit);


/* EN/RU: Consume at most one packet; used reports actual consumed bytes on valid calls including errors.
 * Input disjoint from state/storage, view changes only on OK, MORE retains fragments. Ready/error is sticky.
 * Потребить не более одного пакета: used сообщает фактическое число байт, view меняется лишь при OK; фрагменты сохраняются.
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
FVM_API FVM_SResult fvm_sordered_stream_push(
    FVM_SOrderedStream *stream, const void *input, size_t size, FVM_SOrderedRecord *view, size_t *used);


/* EN/RU: Explicitly discard view/prefix/error without wiping storage; owner first retires borrowed views.
 * Явно сбросить состояние и ошибку без очистки storage, после завершения использования заимствованных views.
 */
FVM_API FVM_SResult fvm_sordered_stream_reset(FVM_SOrderedStream *stream);
#ifdef __cplusplus
}
#endif
#endif
