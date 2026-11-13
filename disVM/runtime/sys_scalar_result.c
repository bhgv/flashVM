/* Copyright (c) 2026 bhgv
 * [GC:] EN: Publish staged native scalar results through the shared cell domain.
 * RU: Публиковать staged native scalar results через общий cell domain.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "raise.h"
#include "sys_capture.h"
#include "managed_owner.h"
#include "disvm/native_process.h"
#include "thread_port.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


/* [GC:] EN: Publish size pure-scalar bytes from private snapshot to a captured
 * mapped result destination. The original caller frame/owners remain rooted.
 * Retry only the publication after BUSY: native effects/completion consumption
 * have already happened and must never be repeated. Every failed claim releases
 * guards before poll; no VM callback/allocation/error occurs with held guards.
 * REFUSED/invalid spans raise without destination effects. Pointer owners require
 * their separate transactional adapter and must not enter this scalar API.
 * RU: Публиковать size pure-scalar bytes из private snapshot в captured mapped
 * destination. Исходные caller frame/owners остаются rooted. После BUSY повторять
 * только publication: native effects/consumption уже произошли. До poll failed
 * claim снимает guards; callbacks/allocation/error под guards нет. REFUSED либо
 * неверный span бросает без записи. Pointer owners требуют отдельного adapter. */
void dvm_sys_scalar_result(const void *snapshot, void *destination, size_t size)
{
    if (!snapshot || !destination || destination == H || !size || size > DVM_SYS_SCALAR_LIMIT ||
        (uintptr_t)destination > UINTPTR_MAX - size)
    {
        error(exBounds);
    }
    for (;;)
    {
        DVM_RangeAccess access = {0};
        DVM_CellResult result = dvm_managed_ranges_begin(NULL, 0, destination, size, &access);
        if (result == DVM_CELL_ACQUIRED)
        {
            memmove(destination, snapshot, size);
            if (!dvm_range_access_end(&access))
            {
                abort();
            }
            return;
        }
        if (result != DVM_CELL_BUSY || !dvm_native_context_poll())
        {
            error(exHeap);
        }
        fvm_port_thread_sleep_us(50);
    }
}
