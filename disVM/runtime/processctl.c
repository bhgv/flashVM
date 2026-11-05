/* Hosted scalar process control through generated NC64 frames. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "raise.h"
#include "sys_capture.h"
#include "processctlmod.h"
#include <limits.h>
#include <stddef.h>


extern int dvm_runtime_cancel_child_wait(Prog *caller, int pid);
extern int dvm_runtime_kill_child(Prog *caller, int pid);
extern int dvm_runtime_prepare_killwait(Prog *caller, int pid);
extern int dvm_runtime_killwait_result(Prog *caller, int pid);


/* Return the current managed process ID through the validated result cell.
 * The caller owns the active VM turn. No allocation or native wait occurs. */
void Processctl_pid(void *frame)
{
    F_Processctl_pid call;
    WORD result;

    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Processctl_pid, ret), sizeof(*call.ret));
    result = currun()->pid;
    memmove(call.ret, &result, sizeof(result));
}


/* Cancel only a current wait of the caller's direct child. frame and its
 * result cell remain rooted for this synchronous, nonblocking operation. */
void Processctl_cancelwait(void *frame)
{
    F_Processctl_cancelwait call;
    WORD result = Processctl_Denied;

    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Processctl_cancelwait, ret), sizeof(*call.ret));
    if (call.pid > 0 && call.pid <= INT_MAX)
    {
        result = dvm_runtime_cancel_child_wait(currun(), (int)call.pid);
    }
    memmove(call.ret, &result, sizeof(result));
}


/* Request direct-child termination. A native continuation keeps ownership
 * until its normal cleanup restores the managed turn. */
void Processctl_kill(void *frame)
{
    F_Processctl_kill call;
    WORD result = Processctl_Denied;

    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Processctl_kill, ret), sizeof(*call.ret));
    if (call.pid > 0 && call.pid <= INT_MAX)
    {
        result = dvm_runtime_kill_child(currun(), (int)call.pid);
    }
    memmove(call.ret, &result, sizeof(result));
}


/* Before publishing a killwait MCALL frame, validate the scalar operands
 * and let the scheduler park its caller. */
int dvm_processctl_call_ready(void *frame)
{
    F_Processctl_killwait call;
    uintptr start, top, extent;

    start = (uintptr)frame;
    top = (uintptr)DVM_R.SP;
    extent = (uintptr)DVM_R.TS;
    if (start < (uintptr)DVM_R.EX || top > extent || start > top || top - start != sizeof(call))
    {
        error(exBounds);
    }
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Processctl_killwait, ret), sizeof(*call.ret));
    return call.pid <= 0 || call.pid > INT_MAX ||
        dvm_runtime_prepare_killwait(currun(), (int)call.pid);
}


/* Consume only the completion belonging to this prepared call. */
void Processctl_killwait(void *frame)
{
    F_Processctl_killwait call;
    WORD result = Processctl_Denied;

    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Processctl_killwait, ret), sizeof(*call.ret));
    if (call.pid > 0 && call.pid <= INT_MAX)
    {
        result = dvm_runtime_killwait_result(currun(), (int)call.pid);
    }
    memmove(call.ret, &result, sizeof(result));
}


/* Register the hosted-only module once during runtime initialization. */
void dvm_processctl_initialize(void)
{
    builtinmod("$Processctl", Processctlmodtab, Processctlmodlen);
}
