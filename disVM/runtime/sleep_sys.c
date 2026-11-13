/* Copyright (c) 2026 bhgv
 * EN: Captured native scalar operations with guarded shared result publication.
 * RU: Captured native scalar operations с guarded shared result publication.
 */


/* Audited scalar blocking callback: OS sleep owns no managed buffer. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "sys_capture.h"

#include "thread_port.h"
#include <limits.h>
#include <stddef.h>


extern int dvm_runtime_guest_sleep(unsigned long milliseconds);


/* [GC:] EN: Capture scalar operands/result from rooted frame before waiting.
 * Native wait touches no managed bytes outside its lease. Publish staged outcome
 * under result cell guards after reentry; publication BUSY never repeats sleep.
 * Invalid native periods/cancellation return -1; nonpositive periods return zero.
 * RU: Захватить scalars/result rooted frame до ожидания. Native wait не трогает
 * managed bytes вне lease. После reentry публиковать outcome под cell guards;
 * BUSY publication не повторяет sleep. Неверный period/отмена дают -1, period<=0 — 0. */
void Sys_sleep(void *frame)
{
    F_Sys_sleep captured;
    WORD result = 0;
    WORD remaining;
    int waited;
    dvm_sys_capture(frame, &captured, sizeof(captured), offsetof(F_Sys_sleep, ret), sizeof(*captured.ret));
    if (captured.period < INT_MIN || captured.period > INT_MAX)
    {
        result = -1;
        dvm_sys_scalar_result(&result, captured.ret, sizeof(result));
        return;
    }
    remaining = captured.period;
    waited = dvm_runtime_guest_sleep(remaining > 0 ? (unsigned long)remaining : 0);
    if (waited < 0)
    {
        result = -1;
        dvm_sys_scalar_result(&result, captured.ret, sizeof(result));
        return;
    }
    while (!waited && remaining > 0)
    {
        unsigned long chunk = remaining > 1000 ? 1000UL : (unsigned long)remaining;
        fvm_port_thread_sleep_us(chunk * 1000UL);
        remaining -= (WORD)chunk;
    }
    dvm_sys_scalar_result(&result, captured.ret, sizeof(result));
}
