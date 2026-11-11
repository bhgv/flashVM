/* Copyright (c) 2026 bhgv
 * EN: Known native table/key finalizers on the shared Fast GC lane.
 * RU: Известные финализаторы таблиц/ключей в параллельном потоке Fast GC.
 */
#ifndef DVM_GC_FAST_TABLE_RETIREMENT_H
#define DVM_GC_FAST_TABLE_RETIREMENT_H


/* [GC:] EN: Return whether borrowed heap has an exact native table/key type.
 * No mutation; other custom finalizers remain outside this policy.
 * RU: Проверить точный нативный тип таблицы/ключей заимствованного heap.
 * Без изменений; другие custom-finalizers не входят в этот путь. */
int dvm_gc_fast_table_eligible(const Heap *heap);


/* [GC:] EN: Finalize claimed ref=0 heap with no lease, waiter or managed fields.
 * Return its native kind. Caller retains/frees heap and type storage after this
 * native-only callback; no managed code, allocation or filesystem I/O is invoked.
 * RU: Завершить heap с ref=0 без lease, ожидающих и управляемых полей.
 * Вернуть вид. Кучу/тип после нативного callback освобождает вызывающий;
 * нет управляемого кода, выделения памяти или файлового I/O. */
int dvm_gc_fast_table_finalize(Heap *heap);
#endif
