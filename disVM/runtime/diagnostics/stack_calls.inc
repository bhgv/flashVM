/* Copyright (c) 2026 bhgv
 * EN: Nonconsuming scalar/byte projections of immutable bounded stack replies.
 * RU: Scalar/byte проекции immutable bounded stack replies без поглощения credit.
 */


/* EN/RU: Validate native bounded geometry; never dereference a managed pointer.
 * Проверить native bounded geometry; не разыменовывать managed pointer. */
static int valid_debug_stack(const DVM_DiagnosticStack *stack)
{
    int index;
    if (stack->sequence > (unsigned long)LLONG_MAX || stack->count < 0 || stack->count > DVM_DIAG_STACK_FRAMES)
    {
        return 0;
    }
    if (!stack->sequence)
    {
        return !stack->count && !stack->flags;
    }
    if ((stack->flags != DVM_STACK_COMPLETE && stack->flags != DVM_STACK_TRUNCATED &&
         stack->flags != DVM_STACK_UNAVAILABLE) ||
        (stack->flags == DVM_STACK_COMPLETE && !stack->count) ||
        (stack->flags == DVM_STACK_TRUNCATED && stack->count != DVM_DIAG_STACK_FRAMES))
    {
        return 0;
    }
    for (index = 0; index < stack->count; index++)
    {
        const DVM_DiagnosticLocation *frame = &stack->frames[index];
        if (frame->sequence != stack->sequence || !(frame->flags & DVM_LOCATION_VALID) || !valid_debug_location(frame))
        {
            return 0;
        }
    }
    return 1;
}


/* EN/RU: Borrow rooted call frame, peek immutable summary and guard scalar output.
 * Заимствовать rooted call frame, читать immutable summary и защищать scalar output. */
void Processctl_residentstack(void *frame)
{
    F_Processctl_residentstack call;
    LONG result[4] = {DVM_DIAG_INVALID, 0, 0, 0}; /* Status, boundary sequence, count, classification. */
    DVM_DiagnosticSample sample;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Processctl_residentstack, ret), sizeof(*call.ret));
    if (sizeof(result) != sizeof(*call.ret))
    {
        error(exBounds);
    }
    if (call.id > 0 && call.ticket > 0)
    {
        result[0] = dvm_resident_debug_peek((unsigned long)call.id, currun()->pid, (unsigned long)call.ticket, &sample);
        if (result[0] == DVM_DIAG_READY)
        {
            if (!valid_debug_stack(&sample.stack))
            {
                result[0] = DVM_DIAG_INVALID;
            }
            else
            {
                result[1] = (LONG)sample.stack.sequence;
                result[2] = sample.stack.count;
                result[3] = sample.stack.flags;
            }
        }
    }
    dvm_sys_scalar_result(result, call.ret, sizeof(result));
}


/* EN/RU: Peek checked frame geometry; invalid/non-Ready results have zero payload.
 * Читать checked frame geometry; invalid/non-Ready results дают zero payload. */
void Processctl_residentframe(void *frame)
{
    F_Processctl_residentframe call;
    LONG result[7] = {DVM_DIAG_INVALID, 0, 0, 0, 0, 0, 0}; /* Status and frame location, matching residentlocation. */
    DVM_DiagnosticSample sample;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Processctl_residentframe, ret), sizeof(*call.ret));
    if (sizeof(result) != sizeof(*call.ret))
    {
        error(exBounds);
    }
    if (call.id > 0 && call.ticket > 0 && call.frame >= 0 && call.frame < DVM_DIAG_STACK_FRAMES)
    {
        result[0] = dvm_resident_debug_peek((unsigned long)call.id, currun()->pid, (unsigned long)call.ticket, &sample);
        if (result[0] == DVM_DIAG_READY)
        {
            if (!valid_debug_stack(&sample.stack) || call.frame >= sample.stack.count)
            {
                result[0] = DVM_DIAG_INVALID;
            }
            else
            {
                const DVM_DiagnosticLocation *item = &sample.stack.frames[call.frame];
                result[1] = (LONG)item->sequence;
                result[2] = item->next_pc;
                result[3] = item->nprog;
                result[4] = item->name_bytes;
                result[5] = item->path_bytes;
                result[6] = item->flags;
            }
        }
    }
    dvm_sys_scalar_result(result, call.ret, sizeof(result));
}


/* EN/RU: Peek one checked label byte; no credit consumption or live target access.
 * Читать checked byte подписи; без consume credit либо live target access. */
void Processctl_residentframebyte(void *frame)
{
    F_Processctl_residentframebyte call;
    LONG result[2] = {DVM_DIAG_INVALID, 0}; /* Status and unsigned UTF-8 byte. */
    DVM_DiagnosticSample sample;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Processctl_residentframebyte, ret), sizeof(*call.ret));
    if (sizeof(result) != sizeof(*call.ret))
    {
        error(exBounds);
    }
    if (call.id > 0 && call.ticket > 0 && call.frame >= 0 && call.frame < DVM_DIAG_STACK_FRAMES && call.part >= 0 &&
        call.part <= 1 && call.index >= 0)
    {
        result[0] = dvm_resident_debug_peek((unsigned long)call.id, currun()->pid, (unsigned long)call.ticket, &sample);
        if (result[0] == DVM_DIAG_READY)
        {
            if (!valid_debug_stack(&sample.stack) || call.frame >= sample.stack.count)
            {
                result[0] = DVM_DIAG_INVALID;
            }
            else
            {
                const DVM_DiagnosticLocation *item = &sample.stack.frames[call.frame];
                int length = call.part ? item->path_bytes : item->name_bytes;
                if (call.index >= length)
                {
                    result[0] = DVM_DIAG_INVALID;
                }
                else
                {
                    result[1] = (unsigned char)(call.part ? item->path[call.index] : item->name[call.index]);
                }
            }
        }
    }
    dvm_sys_scalar_result(result, call.ret, sizeof(result));
}
