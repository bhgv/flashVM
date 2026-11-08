/* Copyright (c) 2026 bhgv
 *
 * EN: Native byte accounting for Fast GC release packets.
 * RU: Учёт памяти пакетов освобождения Fast GC.
 */
#ifndef DVM_GC_PACKET_BUDGET_H
#define DVM_GC_PACKET_BUDGET_H

#include <stddef.h>

/* EN: Account for storage and count rooted keys within limit. Return charge on success.
 * RU: Учесть storage и count удерживаемых keys в пределах limit. При успехе вернуть charge. */
int dvm_gc_packet_charge(void *const *keys, size_t count, unsigned long storage,
    unsigned long limit, unsigned long *charge);

#endif
