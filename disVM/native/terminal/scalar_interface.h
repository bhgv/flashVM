/* Copyright (c) 2026 bhgv
 * EN: Exchange one compiled scalar export signature before submitting calls.
 * RU: Обмен сигнатурой скалярного экспорта до отправки вызовов.
 */
#ifndef STYX_SCALAR_INTERFACE_H
#define STYX_SCALAR_INTERFACE_H
#include "scalar_route.h"

#define STYX_SCALAR_INTERFACE_CAPACITY 64U /* One 48-byte name and bounded framing. */


/* EN: Borrow the export name and link signature, writing one complete offer to peer.
 * The host supplies the signature from a checked compiled export.
 * RU: Отправить предложение с именем и проверенной сигнатурой экспорта. */
DVM_LaunchDelivery styxscalaroffer(DVM_Provider *peer, unsigned long tag,
    const char *name, uint32_t signature);


/* EN: Check the complete borrowed record against the caller's compiled import.
 * On mismatch return zero without opening or writing any route.
 * RU: Сверить запись с компилированным импортом. При несовпадении вернуть ноль без I/O. */
int styxscalaraccept(unsigned long tag, const void *bytes, size_t size,
    const char *name, uint32_t signature);

/* EN/RU: Submit one call only after accepting the borrowed interface record.
 * Отправить один вызов только после сверки заимствованной записи интерфейса. */
DVM_LaunchDelivery styxscalarcallchecked(DVM_Provider *peer, unsigned long call_tag,
    unsigned long offer_tag, const void *offer, size_t size, const char *name,
    uint32_t signature, int64_t argument);

#endif
