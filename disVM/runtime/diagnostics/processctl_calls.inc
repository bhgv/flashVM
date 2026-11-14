/* Copyright (c) 2026 bhgv
 * EN: Rooted guest diagnostic operations; effects precede guarded result publication.
 * RU: Rooted guest diagnostic operations; effects до guarded result publication.
 */


/* EN: Submit once for this caller; retries of guarded publication never resubmit.
 * RU: Отправить один раз для этого caller; retry публикации не повторяет submit. */
void Processctl_residentrequest(void *frame)
{
    F_Processctl_residentrequest call;
    LONG result[2] = {DVM_DIAG_INVALID, 0}; /* Staged status and lifetime ticket. */
    unsigned long ticket = 0;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Processctl_residentrequest, ret), sizeof(*call.ret));
    if (sizeof(result) != sizeof(*call.ret))
    {
        error(exBounds);
    }
    if (call.id > 0)
    {
        result[0] = dvm_resident_debug_request((unsigned long)call.id, currun()->pid, (int64_t)call.cookie, &ticket);
        result[1] = (LONG)ticket;
    }
    dvm_sys_scalar_result(result, call.ret, sizeof(result));
}


/* EN: Poll/consume once for this caller; zero payload on pending or terminal error.
 * Capture before effects; result remains staged across guarded publication retries.
 * RU: Poll/consume один раз для этого caller; zero payload при pending/error.
 * Capture до effects; result сохранён при retry guarded publication. */
void Processctl_residentreply(void *frame)
{
    F_Processctl_residentreply call;
    LONG result[6] = {DVM_DIAG_INVALID, 0, 0, 0, 0, 0}; /* Status, PID, cookie, three counters. */
    DVM_DiagnosticSample sample;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Processctl_residentreply, ret), sizeof(*call.ret));
    if (sizeof(result) != sizeof(*call.ret))
    {
        error(exBounds);
    }
    if (call.id > 0 && call.ticket > 0)
    {
        result[0] =
            dvm_resident_debug_reply((unsigned long)call.id, currun()->pid, (unsigned long)call.ticket, &sample);
        if (result[0] == DVM_DIAG_READY)
        {
            result[1] = sample.owner_pid;
            result[2] = (LONG)sample.cookie;
            result[3] = (LONG)sample.dispatches;
            result[4] = (LONG)sample.turns;
            result[5] = (LONG)sample.waits;
        }
    }
    dvm_sys_scalar_result(result, call.ret, sizeof(result));
}


/* EN: Forget once for this original requester; no ordinary guest operation is cancelled.
 * RU: Забыть один раз для original requester; обычная guest operation не отменяется. */
void Processctl_residentforget(void *frame)
{
    F_Processctl_residentforget call;
    WORD result = DVM_DIAG_INVALID;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Processctl_residentforget, ret), sizeof(*call.ret));
    if (call.id > 0 && call.ticket > 0)
    {
        result = dvm_resident_debug_forget((unsigned long)call.id, currun()->pid, (unsigned long)call.ticket);
    }
    dvm_sys_scalar_result(&result, call.ret, sizeof(result));
}
