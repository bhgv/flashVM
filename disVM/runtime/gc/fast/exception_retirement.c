/* Copyright (c) 2026 bhgv
 * EN: Fast emergency-string retirement after concurrent reader grace periods.
 * RU: Освобождение резервных строк на Fast после завершения эпох читателей.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "../common/exception_queue.h"
#include "../../exception_host.h"
#include "../../root_registry.h"
#include "disvm/native_owner.h"
#include "disvm/refcount.h"
#include <stdio.h>
#include <stdlib.h>

extern Pool *heapmem; /* Stable shared allocator, alive through collector joins. */


/* [GC:] EN: Consume at most budget emergency-string owners on bound Fast context.
 * The FIFO/root owns each string until a reader grace period completes. Retire
 * only Tstring, never invoke arbitrary managed destructors. Return nonzero after
 * progress or while a grace period needs retry, preventing a retained fallback.
 * A missing world/epoch or failed admission leaves the FIFO unchanged. All
 * producer threads and the allocator outlive this call; only Fast consumes.
 * RU: Освободить до budget резервных строк на привязанном контексте Fast.
 * FIFO и корень владеют строкой до завершения эпохи читателей. Обрабатывать
 * только Tstring, не вызывать произвольные деструкторы. Ненулевой результат
 * сообщает о работе либо ожидании эпохи и предотвращает резервную остановку.
 * Без world/epoch или допуска очередь не меняется. Производители и аллокатор
 * живут до завершения вызова; единственный потребитель — Fast.
 */
unsigned int dvm_runtime_exception_retire_shared(DVM_NativeContext *context, size_t budget)
{
    if (!budget || context != dvm_native_context_current())
    {
        return 0;
    }
    DVM_ExceptionRetirement *entry = dvm_exception_retirement_peek();
    DVM_NativeWorld *world = entry ? entry->world : NULL;
    unsigned long epoch = entry ? entry->epoch : 0;
    if (!world || !epoch || heapmonitor)
    {
        return 0;
    }
    DVM_HeapOwner *owner = dvm_native_owner_current();
    if (!dvm_native_owner_attach_service(owner, world))
    {
        return 0;
    }
    unsigned int work = 0;
    if (dvm_native_owner_try_enter_idle_shared(owner))
    {
        while (budget--)
        {
            entry = dvm_exception_retirement_peek();
            if (!entry || entry->world != world || !entry->epoch)
            {
                break;
            }
            if (!dvm_world_epoch_elapsed(world, entry->epoch))
            {
                work = work ? work : 1;
                break;
            }
            Heap *heap = D2H(entry->message);
            if (heap->t != &Tstring)
            {
                fputs("disvm: invalid emergency string retirement type\n", stderr);
                abort();
            }
            dvm_exception_retirement_take(entry);
            if (!dvm_runtime_root_remove(heap))
            {
                abort();
            }
            DVM_RefcountResult result = dvm_refcount_release_u(&heap->ref);
            if (result == DVM_REFCOUNT_LAST)
            {
                free(entry->message->tmp);
                if (dvm_refcount_release_i(&Tstring.ref) != DVM_REFCOUNT_LIVE)
                {
                    abort();
                }
                poolfree(heapmem, heap);
            }
            else if (result != DVM_REFCOUNT_LIVE)
            {
                abort();
            }
            dvm_exception_retirement_count_shared(dvm_world_has_active_guest(world));
            free(entry);
            work++;
        }
        if (!dvm_native_owner_leave_idle_shared(owner))
        {
            abort();
        }
    }
    if (!dvm_native_owner_detach(owner))
    {
        abort();
    }
    return work;
}
