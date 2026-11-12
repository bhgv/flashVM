/* Copyright (c) 2026 bhgv
 *
 * EN: Native byte accounting for Fast GC release packets.
 * RU: Учёт памяти пакетов освобождения Fast GC.
 */
#ifndef DVM_GC_PACKET_BUDGET_H
#define DVM_GC_PACKET_BUDGET_H

#include <stddef.h>


/* [GC:] EN: Borrow count live keys and account storage/payload within limit.
 * Return one and write charge on success; zero preserves charge and key order.
 * Set optional retry to one only on temporary sorting-allocation failure;
 * never clear a prior retry or mark hard extent/limit refusal retryable.
 * Caller owns references/root protection; no managed ownership changes.
 * RU: Заимствовать count живых keys и учесть storage/объекты в limit.
 * Один записывает charge; ноль сохраняет charge и порядок ключей. Лишь отказ
 * памяти сортировки ставит необязательный retry в один. Не сбрасывать прежний
 * retry и не помечать жёсткий предел временным. Владение ссылками/корнями
 * остаётся у вызывающего; управляемое владение не меняется. */
int dvm_gc_packet_charge(
    void *const *keys, size_t count, unsigned long storage, unsigned long limit, unsigned long *charge, int *retry);

#endif
