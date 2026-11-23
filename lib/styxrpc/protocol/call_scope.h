/* Copyright (c) 2026 bhgv
 * EN: Caller PID envelope around a shaped Styx call request.
 * RU: Оболочка PID клиента для типизированного запроса Styx.
 */
#ifndef STYX_CALL_SCOPE_H
#define STYX_CALL_SCOPE_H
#include <stddef.h>
#include <stdint.h>

#define STYX_SCOPED_OFFER_TAG 91UL /* Shaped export requiring a caller lease. */
#define STYX_SCOPED_CALL_TAG 92UL /* PID and one original call request. */
#define STYX_LOCAL_CONTEXT_TAG 93UL /* Private envelope with a native provider attached. */
#define STYX_CALL_STAGE_TAG 94UL /* Bounded request staged on one open function fid. */
#define STYX_CALL_END_TAG 95UL /* Empty record committing the staged request. */
#define STYX_SCOPED_END_OFFER_TAG 96UL /* Scoped profile with explicit request end. */
#define STYX_CALL_CONTEXT_TAG 97UL /* Initial context path. */
#define STYX_SCOPED_PATH_OFFER_TAG 98UL /* Scoped profile with a separate context record. */
#define STYX_CALL_CHUNK_TAG 99UL /* Additional bytes for an already staged request. */
#define STYX_SCOPED_RECORD_OFFER_TAG 102UL /* PID inside one complete call record. */
#define STYX_CALL_RECORD_FRAMING 3 /* Complete S-record commits the call. */
#define STYX_CALL_PATH_FRAMING 2 /* Context, arguments, end. */
#define STYX_CALL_CONTEXT_SELF ".prog/.self" /* Session-local active caller. */
#define STYX_SCOPED_CALL_OVERHEAD 8U /* Four PID bytes plus S-record framing. */


#define STYX_SCOPED_LARGE_OVERHEAD 32U /* Upper bound for PID and framing at 64 KiB. */


/* EN: Encode a scoped record with the selected profile's explicit payload limit.
 * RU: Кодировать контекстную запись с явным пределом данных выбранного профиля. */
int styxcall_scope_write_bounded(uint32_t pid, const void *call, size_t size, size_t limit,
    void *bytes, size_t capacity, size_t *used);


/* EN: Borrow a scoped payload within the limit, preserving outputs on failure.
 * RU: Заимствовать данные контекста в пределах лимита, сохраняя выходы при отказе. */
int styxcall_scope_read_bounded(const void *bytes, size_t size, size_t limit,
    uint32_t *pid, const void **call, size_t *length);


/* EN: Write a caller envelope for one bounded request using a nonzero PID.
 * RU: Записать оболочку одного ограниченного запроса с ненулевым PID. */
int styxcall_scope_write(uint32_t pid, const void *call, size_t size, void *bytes, size_t capacity, size_t *used);


/* EN: Borrow the exact inner bytes from one complete envelope; publish only on success.
 * RU: Заимствовать вложенные байты полной оболочки; выдать только при успехе. */
int styxcall_scope_read(const void *bytes, size_t size, uint32_t *pid, const void **call, size_t *length);
/* EN: Parse a canonical relative selector. A zero PID selects the current caller.
 * RU: Разобрать канонический относительный путь. Нулевой PID выбирает текущего клиента. */
int styxcall_context(const void *bytes, size_t size, uint32_t *pid);
#endif
