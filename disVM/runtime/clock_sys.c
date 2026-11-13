/* Copyright (c) 2026 bhgv
 * EN: Captured native scalar operations with guarded shared result publication.
 * RU: Captured native scalar operations с guarded shared result publication.
 */


/* Sys clock bridge: copy native frame values before accessing the OS port. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "sys_capture.h"
#include "disvm/clock_port.h"
#include <stddef.h>
#include <stdint.h>


/* EN: Capture rooted frame/result before clock access; preserve result on OS
 * failure. Publish staged WORD bits under shared cell guards, retrying publication
 * only. Counter width follows the matched native WORD, not a wire timestamp.
 * RU: Захватить rooted frame/result до clock access; OS failure сохраняет result.
 * Публиковать staged WORD bits под shared cell guards, повторяя лишь publication.
 * Ширина счётчика равна matching native WORD, это не wire timestamp. */
void Sys_millisec(void *frame)
{
    F_Sys_millisec captured;
    uint64_t milliseconds;
    UWORD bits;
    WORD result;
    dvm_sys_capture(frame, &captured, sizeof(captured), offsetof(F_Sys_millisec, ret), sizeof(*captured.ret));
    if (!dvm_host_clock_ms(&milliseconds))
    {
        error("monotonic clock unavailable");
        return;
    }
    bits = (UWORD)milliseconds;
    memmove(&result, &bits, sizeof(result));
    dvm_sys_scalar_result(&result, captured.ret, sizeof(result));
}
