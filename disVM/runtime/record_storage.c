/* Copyright (c) 2026 bhgv
 * EN: Validate, initialize and publish private record storage in SHARED.
 * RU: Проверка, инициализация и публикация приватных записей в SHARED.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "pool.h"
#include "record_storage.h"
#include "record_new.h"
#include "managed_owner.h"
#include "heap_pool.h"
#include "gc_fields.h"
#include "disvm/refcount.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

extern Pool *heapmem; /* Managed heap pool with its own metadata lock. */


/* Return1 for nonwrapping native address/size geometry; never infer mapping. */
int dvm_record_span(const void *address, size_t size)
{
    return (uintptr_t)address >= sizeof(void *) &&
        (uintptr_t)address <= (uintptr_t)-1-size;
}


/* [GC:] Validate immutable genuine Type shape before initmem/map traversal.
 * Return0 for malformed extent/map,1 valid; no allocation/retention/callback.
 * Caller supplies real descriptor lifetime and mapped storage, not just address. */
int dvm_record_valid_type(Type *type)
{
    int byte;
    int bit;
    if (!dvm_record_span(type, offsetof(Type, map)) || type == H ||
        (uintptr_t)type % sizeof(void *) || type->size < 0 ||
        (ulong)type->size > 0x7fffffff-sizeof(Heap) ||
        dvm_refcount_load_i(&type->ref) <= 0 || type->np < 0 || type->np > type->size/(8*sizeof(void *))+
        (type->size%(8*sizeof(void *)) != 0) ||
        !dvm_record_span(type, offsetof(Type, map)+(size_t)type->np))
    {
        return 0;
    }
    for (byte = 0; byte < type->np; byte++)
    {
        for (bit = 0; bit < 8; bit++)
        {
            if ((type->map[byte] & (0x80U>>bit)) &&
                (size_t)byte*8+bit >= (size_t)type->size/sizeof(void *))
            {
                return 0;
            }
        }
    }
    return 1;
}


/* [GC:] Initialize privately allocated bytes and every mapped slot before rooting.
 * count is already bounded, and a zero-sized element has an empty map. */
void dvm_record_initialize_payload(Type *type, int clear, uchar *data, size_t bytes, WORD count)
{
    WORD index;
    if (clear && bytes)
    {
        memset(data, 0, bytes);
    }
    if (type->np)
    {
        for (index = 0; index < count; index++)
        {
            initmem(type, data);
            data += type->size;
        }
    }
}


/* EN: Snapshot the operand under the same range lock as writers.
 * RU: Снять операнд под той же блокировкой диапазона, что у писателей. */
int dvm_runtime_record_shared_index(const void *source, WORD *index)
{
    DVM_CellDomain *domain = dvm_managed_dispatch_cell_domain();
    DVM_RangeAccess access = {0};
    if (!domain || source == H || !dvm_record_span(source, sizeof(*index)) ||
        dvm_range_access_begin(domain, source, sizeof(*index), NULL, 0, &access) != DVM_CELL_ACQUIRED)
    {
        return 0;
    }
    memmove(index, source, sizeof(*index));
    if (!dvm_range_access_end(&access))
    {
        abort();
    }
    return 1;
}


/* EN: Read or fill an empty owned cell under its canonical range guard.
 * RU: Прочитать или заполнить пустую ячейку под её общей блокировкой. */
static int fill_empty_record(DVM_CellDomain *domain, void *destination, void *value)
{
    DVM_RangeAccess access = {0};
    if (dvm_range_access_begin(domain, destination, sizeof(void *), NULL, 0, &access) != DVM_CELL_ACQUIRED)
    {
        return 0;
    }
    void *previous;
    memmove(&previous, destination, sizeof(previous));
    if (previous == H && value)
    {
        memmove(destination, &value, sizeof(value));
    }
    if (!dvm_range_access_end(&access))
    {
        abort();
    }
    return previous == H;
}


/* EN: Construct a private record and publish it without retiring an old owner.
 * RU: Собрать приватную запись и опубликовать её без снятия старой ссылки. */
int dvm_runtime_record_try_shared(void *destination, Type *type, int clear)
{
    DVM_CellDomain *domain = dvm_managed_dispatch_cell_domain();
    if (!domain || heapmonitor || !dvm_record_valid_type(type) || destination == H ||
        !dvm_record_span(destination, sizeof(void *)) || !fill_empty_record(domain, destination, NULL) ||
        dvm_refcount_retain_i(&type->ref) != DVM_REFCOUNT_LIVE)
    {
        return 0;
    }
    Heap *header = dvm_heap_pool_try_alloc(heapmem, sizeof(Heap) + (size_t)type->size);
    if (header)
    {
        header->t = type;
        header->ref = 1;
        dvm_runtime_gc_store(&header->color, dvm_runtime_gc_load(&mutator));
        dvm_record_initialize_payload(type, clear, H2D(uchar *, header), (size_t)type->size, 1);
        Setmark(header);
        if (fill_empty_record(domain, destination, H2D(void *, header)))
        {
            return 1;
        }
        poolfree(heapmem, header);
    }
    if (dvm_refcount_release_i(&type->ref) != DVM_REFCOUNT_LIVE)
    {
        abort();
    }
    return 0;
}


