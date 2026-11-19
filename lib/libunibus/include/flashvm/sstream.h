/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: incremental checked S-record input and caller-buffer lifetime.
 * RU: инкрементальный проверяемый ввод S-записей и время жизни буфера.
 */
/* Caller-buffered incremental S-record reception; no transport, allocation or VM dependency. */
#ifndef FLASHVM_SSTREAM_H
#define FLASHVM_SSTREAM_H           /* Guard the C89 incremental decoder ABI. */
#include "srecord.h"
#define FVM_SSTREAM_MIN_CAPACITY 3U /* Smallest checksum + length + tag record. */
#ifdef __cplusplus
extern "C"
{
#endif
/* Mutable native decoder for exactly one channel direction; fields are private policy state. */
typedef struct FVM_SStream
{
    size_t struct_size;    /* Exact native size filled by init. */
    unsigned char *buffer; /* Caller-owned disjoint receive storage, stable until disposal. */
    size_t capacity;       /* Accessible bytes in buffer. */
    size_t limit;          /* Body quota, 1..SRECORD_BODY_MAX. */
    size_t filled;         /* Bytes retained from current record. */
    size_t expected;       /* Total frame extent once canonical framing is known, otherwise zero. */
    int ready;             /* A verified borrowed view is live; push cannot overwrite it. */
    FVM_SResult error;     /* Latched parse/quota/checksum error until explicit reset. */
} FVM_SStream;

/* Initialize without touching buffer; capacity may be below limit and then enforces a tighter
     * bound. No allocation, locks, VM/GC or I/O. Output/storage must not overlap. */


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
FVM_API FVM_SResult fvm_sstream_init(FVM_SStream *, void *buffer, size_t capacity, size_t limit);


/* Consume at most one record, stopping before trailing input. Input must not overlap
     * buffer/state. used is consumed input even on MORE or a latched data error; unchanged on
     * invalid arguments. view changes only on OK and borrows buffer until reset/disposal.
     * Preserve/append only bytes not consumed; consumed bytes are already retained. Zero input
     * returns MORE for an incomplete record. After OK, push returns INVALID with used=0 until
     * reset; errors are latched with used=0 on retry. Bounds are enforced as soon as framing is
     * available, before copying an advertised body. */


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
FVM_API FVM_SResult fvm_sstream_push(FVM_SStream *, const void *, size_t size, FVM_SRecord *view, size_t *used);


/* Invalidate an outstanding view and discard prefix/error without wiping caller bytes.
     * Owner must quiesce users first; this does not resynchronize a transport or cancel remote
     * work. */
FVM_API FVM_SResult fvm_sstream_reset(FVM_SStream *);
#ifdef __cplusplus
}
#endif
#endif
