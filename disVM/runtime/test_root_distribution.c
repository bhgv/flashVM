/* Copyright (c) 2026 bhgv
 * [GC:] EN: Strided native identities keep root chains short without losing occurrences.
 * RU: Адреса native-объектов с постоянным шагом не удлиняют цепочки и не теряют корни.
 */
#include "root_registry.h"
#include "root_registry_private.h"
#include "root_batch.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define KEYS 8192   /* Distinct real native objects, not managed heaps. */
#define STRIDE 2048 /* Every key collided in the previous 32-bucket shifted-address index. */


/* EN: Terminate on false condition with borrowed message; own no managed heap.
 * RU: При ложном condition завершиться с заимствованным message; управляемой кучи нет. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL root distribution: %s\n", message);
        exit(12);
    }
}


/* [GC:] EN: Create actual strided native keys, inspect real bucket identities
 * under its mutex, then reconcile ordinary/batch duplicates through removal.
 * No root visitor or managed GC runs in this standalone process. Return zero
 * after complete cleanup; protocol/allocation failures terminate explicitly.
 * RU: Создать настоящие native-ключи с постоянным шагом, проверить реальные
 * корзины под мьютексом, сверить обычные/пакетные дубликаты при удалении. Здесь
 * нет visitor и управляемого GC. Вернуть ноль после очистки; ошибки завершают тест. */
int main(void)
{
    unsigned char *arena = malloc((size_t)KEYS * STRIDE);
    void **keys = malloc(2 * KEYS * sizeof(*keys));
    RootEntry ***heads = calloc(KEYS, sizeof(*heads));
    unsigned *counts = calloc(KEYS, sizeof(*counts));
    require(arena && keys && heads && counts, "native scratch allocation");
    for (size_t index = 0; index < KEYS; index++)
    {
        keys[index] = arena + index * STRIDE;
        keys[index + KEYS] = keys[index];
        require(dvm_runtime_root_add(keys[index]), "ordinary native root publication");
    }
    unsigned used = 0;
    unsigned maximum = 0;
    FVM_PortSync *mutex = dvm_root_registry_sync(0);
    require(mutex != NULL, "published registry mutex");
    fvm_port_sync_lock(mutex);
    for (size_t index = 0; index < KEYS; index++)
    {
        RootEntry **head = dvm_root_registry_bucket(keys[index]);
        unsigned found = 0;
        while (found < used && heads[found] != head)
        {
            found++;
        }
        if (found == used)
        {
            heads[used++] = head;
        }
        counts[found]++;
        if (counts[found] > maximum)
        {
            maximum = counts[found];
        }
    }
    fvm_port_sync_unlock(mutex);
    require(used >= KEYS / 4 && maximum <= 64, "strided identities must not collapse the root index");
    DVM_RootBatch *batch = dvm_runtime_root_batch_create(2 * KEYS);
    require(batch && dvm_runtime_root_batch_publish(batch, keys, 2 * KEYS), "duplicate batch publication");
    for (size_t index = 0; index < KEYS; index++)
    {
        require(dvm_runtime_root_count(keys[index]) == 3, "ordinary and duplicate batch occurrences");
        require(dvm_runtime_root_remove(keys[index]) && dvm_runtime_root_count(keys[index]) == 2,
                "ordinary removal preserves both batch roots");
    }
    require(dvm_runtime_root_batch_clear(batch) && dvm_runtime_root_batch_destroy(&batch), "batch cleanup");
    for (size_t index = 0; index < KEYS; index++)
    {
        require(!dvm_runtime_root_count(keys[index]), "no root occurrence lost during unlink");
    }
    printf("PASS root distribution keys=%u buckets=%u max_chain=%u occurrences=%u\n", KEYS, used, maximum, 3 * KEYS);
    free(counts);
    free(heads);
    free(keys);
    free(arena);
    return 0;
}
