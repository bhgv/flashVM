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


/* Implement Sys.sleep with captured scalar operands and result location. frame
 * remains rooted in the published native call stack. No managed memory is
 * accessed while the owner is outside the heap. Native32 range errors return
 * -1; nonpositive periods return zero, matching the existing owning call shape.
 * A released sleep can be cancelled by PID or shutdown and returns -1.
 * An inline sleep retains the ordinary OS delay. */
void Sys_sleep(void *frame)
{
    F_Sys_sleep captured;
    WORD result = 0;
    WORD remaining;
    int waited;
    dvm_sys_capture(frame,&captured,sizeof(captured),
        offsetof(F_Sys_sleep,ret),sizeof(*captured.ret));
    if (captured.period < INT_MIN || captured.period > INT_MAX)
    {
        result = -1;
        memmove(captured.ret,&result,sizeof(result));
        return;
    }
    remaining = captured.period;
    waited = dvm_runtime_guest_sleep(remaining > 0 ? (unsigned long)remaining : 0);
    if (waited < 0)
    {
        result = -1;
        memmove(captured.ret, &result, sizeof(result));
        return;
    }
    while (!waited && remaining > 0)
    {
        unsigned long chunk = remaining > 1000 ? 1000UL : (unsigned long)remaining;
        fvm_port_thread_sleep_us(chunk*1000UL);
        remaining -= (WORD)chunk;
    }
    memmove(captured.ret,&result,sizeof(result));
}
