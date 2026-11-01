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


/* Implement the existing Sys.millisec entry without GUI, scheduler waits or
 * heap allocation. Capture the caller's result cell first; report OS failure
 * through the normal VM error path and preserve the cell on failure. The
 * counter uses this runtime's WORD width and wraps as a bit pattern. */
void Sys_millisec(void *frame)
{
    F_Sys_millisec captured;
    uint64_t milliseconds;
    UWORD bits;
    WORD result;
    dvm_sys_capture(frame, &captured, sizeof(captured),
                    offsetof(F_Sys_millisec, ret), sizeof(*captured.ret));
    if (!dvm_host_clock_ms(&milliseconds))
    {
        error("monotonic clock unavailable");
        return;
    }
    bits = (UWORD)milliseconds;
    memmove(&result, &bits, sizeof(result));
    memmove(captured.ret, &result, sizeof(result));
}
