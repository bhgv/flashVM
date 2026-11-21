/* Copyright (c) 2026 bhgv
 * EN: Framed composition of a signed int and big for mounted Dis calls.
 * RU: Кадровая композиция знаковых int и big для смонтированных вызовов Dis.
 */
#ifndef STYX_TUPLE_RECORD_H
#define STYX_TUPLE_RECORD_H
#include <stddef.h>
#include <stdint.h>

#define STYX_FUNCTION_FRAME_CAPACITY 64U /* Includes outer frame and up to three nested records. */

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
#endif
