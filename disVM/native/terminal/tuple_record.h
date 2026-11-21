/* Copyright (c) 2026 bhgv
 * EN: Framed composition of a signed int and big for mounted Dis calls.
 * RU: Кадровая композиция знаковых int и big для смонтированных вызовов Dis.
 */
#ifndef STYX_TUPLE_RECORD_H
#define STYX_TUPLE_RECORD_H
#include <stddef.h>
#include <stdint.h>

#define STYX_FUNCTION_FRAME_CAPACITY 64U /* Bound for a complete mounted function record. */

typedef struct StyxTuple
{
    int32_t narrow; /* Limbo int. */
    int64_t wide; /* Limbo big. */
} StyxTuple;


/* EN: Encode an exact tuple argument or result. Scoped tag requires a nonzero PID.
 * RU: Закодировать аргумент или результат. Тег с контекстом требует ненулевой PID. */
int styxtuplewrite(unsigned long tag, uint32_t pid, const StyxTuple *value,
    void *bytes, size_t capacity, size_t *used);

/* EN: Decode complete nested records and publish outputs only after validation.
 * RU: Прочитать полные вложенные записи и выдать поля после проверки. */
int styxtupleread(unsigned long tag, const void *bytes, size_t size, uint32_t *pid, StyxTuple *value);


/* EN: A complete result followed by its copy-back state.
 * RU: Полный результат и обновлённое состояние. */
typedef struct StyxRefReply
{
    StyxTuple result; /* Ordinary Limbo return value. */
    StyxTuple updated; /* Candidate values for the caller's referenced object. */
} StyxRefReply;

/* EN: Encode reply in bytes up to capacity; used reports the encoded length.
 * RU: Записать reply в bytes объёмом capacity; used получает длину записи. */
int styxrefreplywrite(const StyxRefReply *reply, void *bytes, size_t capacity, size_t *used);
/* EN: Decode exactly size bytes into reply only after both tuples pass validation.
 * RU: Прочитать ровно size байтов в reply после проверки обоих кортежей. */
int styxrefreplyread(const void *bytes, size_t size, StyxRefReply *reply);
#endif
