/* Copyright (c) 2026 bhgv
 * EN: Exact native Bigtable owner classification for GC, without exporting payloads.
 * RU: Точные типы нативных владельцев Bigtable для GC без экспорта payload.
 */
#ifndef DVM_BIGTABLE_OWNER_H
#define DVM_BIGTABLE_OWNER_H


/* [GC:] EN: Runtime-lifetime owner kinds, never guest-provided type tags.
 * RU: Виды владельцев времени жизни runtime, не гостевые теги типов. */
enum
{
    DVM_BIGTABLE_NATIVE_TABLE = 1, /* Scalar table with a quiescent native guard at last release. */
    DVM_BIGTABLE_NATIVE_KEYS = 2 /* Independent immutable native key snapshot. */
};


/* [GC:] EN: Classify borrowed heap by startup-initialized exact type identity.
 * Return the native kind or zero. No payload access, allocation or mutation.
 * RU: Проверить heap по точному типу, установленному при старте.
 * Вернуть вид либо ноль. Без доступа к payload, выделения и изменения. */
int dvm_bigtable_native_kind(const Heap *heap);


/* [GC:] EN: Verify native lease/FIFO metadata is empty before last-owner cleanup.
 * heap is already claimed by GC with no live borrowers. Take only the scheduler
 * metadata mutex; no native payload is freed or entered here. Return 1 if quiet.
 * RU: Проверить отсутствие lease/FIFO перед очисткой последнего владельца.
 * heap уже захвачен GC без живых заёмщиков. Взять только мьютекс метаданных;
 * не входить в payload и не освобождать его. Вернуть 1 при покое. */
int dvm_bigtable_native_quiet(const Heap *heap);
#endif
