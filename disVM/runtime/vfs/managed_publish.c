/* Copyright (c) 2026 bhgv
 * [GC:] EN: Move decoded managed return values into the caller's mapped frame.
 * RU: Передавать декодированные managed-значения в mapped кадр вызывающего.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "raise.h"
#include "managed_value.h"
#include "managed_owner.h"
#include "thread_port.h"
#include "disvm/refcount.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


/* [GC:] EN: Transfer staged managed owners into a checked return range. Retry
 * only a busy range admission; keep all decoded objects temporarily rooted.
 * RU: Передать staged managed owners в проверенный диапазон возврата. При
 * занятом диапазоне повторять допуск, сохраняя временные корни объектов.
 */
int dvm_styx_value_frame_move(DVM_StyxValueFrame *frame, void *destination, size_t size)
{
    if (!frame || size != frame->layout.size || (size && (!destination || destination == H ||
        (uintptr_t)destination > UINTPTR_MAX - size)))
    {
        return 0;
    }
    void *previous[STYX_CALL_VALUES] = {0};
    if (size)
    {
        for (;;)
        {
            DVM_RangeAccess access = {0};
            DVM_CellResult result = dvm_managed_ranges_begin(NULL, 0, destination, size, &access);
            if (result == DVM_CELL_ACQUIRED)
            {
                for (size_t i = 0; i < frame->layout.count; i++)
                {
                    if (frame->layout.pointers & (1U << i))
                    {
                        memcpy(&previous[i], (unsigned char *)destination + frame->layout.offset[i],
                            sizeof(previous[i]));
                    }
                }
                memmove(destination, frame->words, size);
                if (!dvm_range_access_end(&access))
                {
                    abort();
                }
                break;
            }
            if (result != DVM_CELL_BUSY || !dvm_native_context_poll())
            {
                return 0;
            }
            fvm_port_thread_sleep_us(50);
        }
    }
    for (size_t i = 0; i < frame->layout.count; i++)
    {
        if (previous[i] && previous[i] != H)
        {
            destroy(previous[i]);
        }
        if (frame->roots & (1U << i))
        {
            void *object;
            memcpy(&object, (unsigned char *)frame->words + frame->layout.offset[i], sizeof(object));
            ptrdel(D2H(object));
        }
    }
    memset(frame, 0, sizeof(*frame));
    return 1;
}


/* [GC:] EN: Transfer decoded owners into a fresh, mapped gateway argument
 * area. The frame belongs to this resident and is still private. Its pointer
 * cells must already contain H. On refusal arguments remain owned by caller.
 * RU: Передать декодированные ссылки в новый кадр шлюза текущего resident.
 * Слоты указателей до передачи содержат H. При отказе ссылки остаются у caller. */
int dvm_styx_owner_frame_init(DVM_StyxValueFrame *arguments, Modlink *target,
    void *frame, size_t extent)
{
    const size_t target_offset = 64;
    const size_t values_offset = 72;
    if (!arguments || !target || target == H || !frame || frame == H ||
        extent < values_offset || arguments->layout.size > extent - values_offset ||
        arguments->layout.count > STYX_CALL_VALUES ||
        arguments->layout.size > sizeof(arguments->words))
    {
        return 0;
    }
    unsigned char *bytes = frame;
    void *previous;
    memcpy(&previous, bytes + target_offset, sizeof(previous));
    if (previous != H)
    {
        return 0;
    }
    for (size_t i = 0; i < arguments->layout.count; i++)
    {
        if (arguments->layout.pointers & (1U << i))
        {
            size_t offset = arguments->layout.offset[i];
            if (offset > arguments->layout.size ||
                sizeof(previous) > arguments->layout.size - offset)
            {
                return 0;
            }
            memcpy(&previous, bytes + values_offset + offset, sizeof(previous));
            if (previous != H)
            {
                return 0;
            }
        }
    }
    Heap *heap = D2H(target);
    if (dvm_refcount_retain_u(&heap->ref) != DVM_REFCOUNT_LIVE)
    {
        return 0;
    }
    Setmark(heap);
    memcpy(bytes + target_offset, &target, sizeof(target));
    memcpy(bytes + values_offset, arguments->words, arguments->layout.size);
    for (size_t i = 0; i < arguments->layout.count; i++)
    {
        if (arguments->roots & (1U << i))
        {
            void *object;
            memcpy(&object, bytes + values_offset + arguments->layout.offset[i], sizeof(object));
            ptrdel(D2H(object));
        }
    }
    memset(arguments, 0, sizeof(*arguments));
    return 1;
}
