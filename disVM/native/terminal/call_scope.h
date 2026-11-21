/* Copyright (c) 2026 bhgv
 * EN: Caller PID envelope around a shaped Styx value request.
 * RU: Оболочка PID клиента для типизированного запроса Styx.
 */
#ifndef STYX_CALL_SCOPE_H
#define STYX_CALL_SCOPE_H
#include <stddef.h>
#include <stdint.h>

#define STYX_SCOPED_OFFER_TAG 91UL /* Shaped export requiring a caller lease. */
#define STYX_SCOPED_CALL_TAG 92UL /* PID and one original value request. */
#define STYX_SCOPED_CALL_OVERHEAD 8U /* Four PID bytes plus S-record framing. */


/* EN: Write a caller envelope for one bounded request using a nonzero PID.
 * RU: Записать оболочку одного ограниченного запроса с ненулевым PID. */
int styxcall_scope_write(uint32_t pid, const void *call, size_t size, void *bytes, size_t capacity, size_t *used);


/* EN: Borrow the exact inner bytes from one complete envelope; publish only on success.
 * RU: Заимствовать вложенные байты полной оболочки; выдать только при успехе. */
int styxcall_scope_read(const void *bytes, size_t size, uint32_t *pid, const void **call, size_t *length);
#endif
