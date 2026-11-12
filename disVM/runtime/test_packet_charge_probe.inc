/* Copyright (c) 2026 bhgv
 * [GC:] EN: Exercise actual alias sorting under a deliberately tight native charge limit.
 * RU: Настоящая сортировка повторных ссылок с намеренно малым лимитом native-учёта.
 * Private fixture fragment; the real allocator, owners and GC remain in use.
 */
#include "gc/fast/packet_budget.h"


/* [GC:] EN: Borrow live keys and account them using the production implementation.
 * charge/retry obey its output contract; no roots/references change here.
 * RU: Заимствовать живые keys и учесть их настоящей реализацией. charge/retry
 * сохраняют исходный контракт; корни и ссылки здесь не меняются. */
int __real_dvm_gc_packet_charge(
    void *const *keys, size_t count, unsigned long storage, unsigned long limit, unsigned long *charge, int *retry);


/* [GC:] EN: Clamp only this selected real alias packet to storage plus its one
 * unique allocation, forcing the production deduplication path without huge
 * managed allocations. Validate hard-limit/optional-retry behavior separately.
 * The malloc wrapper may refuse only the actual sorting vector, eight passes.
 * RU: Ограничить лишь выбранный настоящий пакет повторных ссылок размером
 * storage и единственного объекта, вызывая настоящий dedup без огромной кучи.
 * Отдельно проверить жёсткий лимит и optional retry. malloc может отказать
 * только настоящему вектору сортировки в восьми проходах. */
int __wrap_dvm_gc_packet_charge(
    void *const *keys, size_t count, unsigned long storage, unsigned long limit, unsigned long *charge, int *retry)
{
    if (!selected || count != Owners || !count || keys[0] != children[0])
    {
        return __real_dvm_gc_packet_charge(keys, count, storage, limit, charge, retry);
    }
    require((target->t == &Tarray) == ChargeShape, "actual selected record/array parent shape");
    unsigned long untouched = 123;
    int hard_retry = 0;
    require(storage && !__real_dvm_gc_packet_charge(keys, count, storage, storage - 1, &untouched, &hard_retry) &&
                untouched == 123 && !hard_retry,
            "hard charge refusal remains permanent and output-neutral");
    hard_retry = 1;
    require(!__real_dvm_gc_packet_charge(keys, count, storage, storage - 1, &untouched, &hard_retry) &&
                untouched == 123 && hard_retry,
            "hard charge refusal preserves a prior retry");
    charge_sorting = 1;
    int result =
        __real_dvm_gc_packet_charge(keys, count, storage, storage + poolmsize(heapmem, keys[0]), charge, retry);
    charge_sorting = 0;
    return result;
}
