/* Copyright (c) 2026 bhgv
 *
 * EN: Count payload allocations without changing a release packet's owner order.
 * RU: Учёт объектов кучи с сохранением порядка ссылок в пакете освобождения.
 */
#include "lib9.h"
#include "pool.h"
#include "packet_budget.h"
#undef qsort /* Use the host C library sort for native pointer metadata. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern Pool *heapmem; /* Borrowed while packet owners pin the referenced blocks. */


/* EN: Compare addresses stored in the left and right array entries.
 * RU: Сравнить адреса в элементах массива left и right. */
static int compare_block_keys(const void *left, const void *right)
{
    uintptr_t first = (uintptr_t) * (void *const *)left;
    uintptr_t second = (uintptr_t) * (void *const *)right;
    return (first > second) - (first < second);
}


/* EN: Add the sizes of count live keys to total within limit.
 * If sorted is set, adjacent duplicate keys describe a single allocation.
 * RU: Добавить размеры count живых объектов keys к total в пределах limit.
 * При sorted соседние одинаковые ключи обозначают один объект. */
static int charge_blocks(void *const *keys, size_t count, unsigned long limit, int sorted, unsigned long *total)
{
    for (size_t index = 0; index < count; index++)
    {
        if (sorted && index && keys[index] == keys[index - 1])
        {
            continue;
        }
        unsigned long size = poolmsize(heapmem, keys[index]);
        if (size > limit - *total)
        {
            return 0;
        }
        *total += size;
    }
    return 1;
}


/* [GC:] EN: Add storage and payload charges for count keys within limit.
 * Keep the caller's key order. Return the total in charge only on success.
 * Optional retry records native sorting OOM, preserving any prior retry.
 * RU: Учесть storage и память count объектов keys в пределах limit.
 * Порядок ключей сохраняется. Записать итог в charge только при успехе.
 * Необязательный retry отмечает нехватку памяти сортировки без сброса прежнего. */
int dvm_gc_packet_charge(
    void *const *keys, size_t count, unsigned long storage, unsigned long limit, unsigned long *charge, int *retry)
{
    if (storage > limit || count > SIZE_MAX / sizeof(void *))
    {
        return 0;
    }
    unsigned long total = storage;
    if (charge_blocks(keys, count, limit, 0, &total))
    {
        *charge = total;
        return 1;
    }
    void **sorted = malloc(count * sizeof(*sorted));
    if (!sorted)
    {
        if (retry)
        {
            *retry = 1;
        }
        return 0;
    }
    memcpy(sorted, keys, count * sizeof(*sorted));
    qsort(sorted, count, sizeof(*sorted), compare_block_keys);
    total = storage;
    int fits = charge_blocks(sorted, count, limit, 1, &total);
    free(sorted);
    if (fits)
    {
        *charge = total;
    }
    return fits;
}
