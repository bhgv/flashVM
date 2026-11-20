/* Copyright (c) 2026 bhgv
 * EN: One signed scalar request or reply carried by a Unibus S-record.
 * RU: Скалярный знаковый запрос или ответ в S-записи Unibus.
 */
#ifndef STYX_SCALAR_RECORD_H
#define STYX_SCALAR_RECORD_H
#include <stddef.h>
#include <stdint.h>

#define STYX_SCALAR_RECORD_CAPACITY 32U /* Sufficient bytes for any scalar record. */

typedef enum StyxScalarKind
{
    STYX_SCALAR_CALL = 1,  /* Client argument. */
    STYX_SCALAR_VALUE = 2, /* Owner result. */
    STYX_SCALAR_ERROR = 3  /* Owner error code. */
} StyxScalarKind;


/* EN/RU: Encode one complete record, returning 1 on success and 0 on refusal.
 * Закодировать одну полную запись, вернуть 1 при успехе и 0 при отказе. */
int styxscalarwrite(unsigned long tag, StyxScalarKind kind, int64_t value,
    void *bytes, size_t capacity, size_t *used);


/* EN/RU: Decode an exact matching record without changing outputs on refusal.
 * Прочитать одну совпадающую запись, сохранив выходные значения при отказе. */
int styxscalarread(unsigned long tag, const void *bytes, size_t size,
    StyxScalarKind *kind, int64_t *value);


/* EN/RU: Invoke an admitted scalar method once and encode its value or error reply.
 * Вызвать допущенный скалярный метод один раз и закодировать ответ или ошибку. */
int styxscalarrespond(unsigned long request_tag, const void *request, size_t request_size,
    unsigned long reply_tag, int (*method)(void *, int64_t, int64_t *), void *state,
    void *reply, size_t capacity, size_t *used);

/* EN/RU: Encode or decode one complete signed-int call with a nonzero process identity.
 * Закодировать или прочитать вызов со знаковым int и ненулевым PID. */
int styxscalarcallerwrite(uint32_t pid, int32_t argument, void *bytes, size_t capacity, size_t *used);
int styxscalarcallerread(const void *bytes, size_t size, uint32_t *pid, int64_t *argument);
#endif
