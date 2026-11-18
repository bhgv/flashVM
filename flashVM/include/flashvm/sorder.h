/* Copyright (c) 2026 bhgv
 * EN: Compact unsigned64 sequence prefix with CRC-5, checked S-record packets and incremental reception.
 * RU: Компактный 64-битный префикс с CRC-5, проверка пакетов S-записей и приём по частям.
 */
#ifndef FLASHVM_SORDER_H
#define FLASHVM_SORDER_H
#include "sstream.h"
#define FVM_SORDER_PREFIX_MAX 9U /* One CRC/length byte plus one to eight unsigned big-endian bytes. */
#ifdef __cplusplus
extern "C" {
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
    size_t encoded_size; /* Prefix plus exactly one complete S-record. */
} FVM_SOrderedRecord;


/* EN/RU: Encode canonical prefix, S_OK/LIMIT/INVALID, outputs/used unchanged on failure; no heap, C89 ABI.
 * Header low3=number_bytes-1, high5=CRC over those3 bits followed by number bytes. MSB first, poly0x05,
 * init0x1f/xorout0x1f, no reflection or padding. Zero is one zero byte, leading zero bytes forbidden.
 * Каноническое кодирование префикса: младшие 3 бита — длина минус один, старшие 5 — CRC длины и номера; отказ не меняет результат.
 */
FVM_API FVM_SResult fvm_sordinal_write(const FVM_SOrdinal *value, void *output, size_t capacity, size_t *used);


/* EN/RU: Decode one canonical prefix, MORE until complete; outputs/used unchanged except on OK.
 * CRC is a corruption check, not authentication or a guarantee that prefix belongs to another valid body.
 * Прочитать префикс; MORE до завершения. Результат и used меняются лишь при OK; CRC не удостоверяет отправителя.
 */
FVM_API FVM_SResult fvm_sordinal_read(const void *input, size_t size, FVM_SOrdinal *value, size_t *used);


/* EN/RU: Encode prefix plus one original S-record atomically, bounded by original global body limit.
 * Nonoverlapping borrowed inputs/output, no heap; output/used unchanged on any refusal.
 * Атомарно кодировать префикс и одну исходную S-запись; буферы не пересекаются, выделения памяти нет.
 */
FVM_API FVM_SResult fvm_sordered_write(const FVM_SOrdinal *ordinal, unsigned long tag, const void *bytes,
    size_t size, void *output, size_t capacity, size_t *used);


/* EN/RU: Validate prefix and exactly one S-record, limit applies to S-record body; trailing packets remain unread.
 * Borrow input through view lifetime; view/used unchanged on MORE/error, original CRC still checked.
 * Проверить префикс и одну S-запись; следующие пакеты не потребляются, MORE/ошибка не меняют view/used.
 */
FVM_API FVM_SResult fvm_sordered_read(const void *input, size_t size, size_t limit,
    FVM_SOrderedRecord *view, size_t *used);


/* EN/RU: Caller-buffered decoder, private fields after init; no transport, VM, heap or implicit resynchronization.
 * Декодер на буфере caller; после init поля закрыты для записи. Без transport, VM, heap и поиска новой границы.
 */
typedef struct FVM_SOrderedStream
{
    size_t struct_size; /* Exact initialized native descriptor size. */
    unsigned char prefix[FVM_SORDER_PREFIX_MAX]; /* At most9 copied prefix bytes. */
    size_t filled;      /* Prefix bytes retained. */
    size_t expected;    /* Exact prefix extent after first byte, zero before it. */
    FVM_SOrdinal ordinal; /* Validated copied sequence once prefix complete. */
    FVM_SStream stream; /* Original S-record decoder over caller-owned disjoint storage. */
    int ready;          /* One verified borrowed view is outstanding. */
    FVM_SResult error;  /* Latched framing/checksum error until explicit reset. */
} FVM_SOrderedStream;


/* EN/RU: Initialize decoder over borrowed storage, unchanged descriptor on refusal; same body bounds as SStream.
 * Инициализировать декодер на заимствованном storage с границами SStream; отказ не меняет descriptor.
 */
FVM_API FVM_SResult fvm_sordered_stream_init(FVM_SOrderedStream *stream, void *storage, size_t capacity, size_t limit);


/* EN/RU: Consume at most one packet; used reports actual consumed bytes on valid calls including errors.
 * Input disjoint from state/storage, view changes only on OK, MORE retains fragments. Ready/error is sticky.
 * Потребить не более одного пакета: used сообщает фактическое число байт, view меняется лишь при OK; фрагменты сохраняются.
 */
FVM_API FVM_SResult fvm_sordered_stream_push(FVM_SOrderedStream *stream, const void *input, size_t size,
    FVM_SOrderedRecord *view, size_t *used);


/* EN/RU: Explicitly discard view/prefix/error without wiping storage; owner first retires borrowed views.
 * Явно сбросить состояние и ошибку без очистки storage, после завершения использования заимствованных views.
 */
FVM_API FVM_SResult fvm_sordered_stream_reset(FVM_SOrderedStream *stream);
#ifdef __cplusplus
}
#endif
#endif
