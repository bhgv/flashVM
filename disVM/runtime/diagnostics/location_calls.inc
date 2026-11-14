/* Copyright (c) 2026 bhgv
 * EN: Nonconsuming scalar projections of a bounded copied location reply.
 * RU: Scalar-проекции копии location reply без поглощения ticket.
 */


/* EN: Validate copied native geometry before projecting fields/bytes. This
 * checks bounds, not arbitrary pointer provenance; input has no live pointers.
 * RU: Проверить native geometry копии перед выдачей fields/bytes. Проверяются
 * bounds, а не происхождение pointers; input не содержит live pointers. */
static int valid_debug_location(const DVM_DiagnosticLocation *location)
{
    if (location->sequence > (unsigned long)LLONG_MAX || location->name_bytes < 0 ||
        location->name_bytes >= DVM_DIAG_NAME_BYTES || location->path_bytes < 0 ||
        location->path_bytes >= DVM_DIAG_PATH_BYTES ||
        (location->flags & ~(DVM_LOCATION_VALID | DVM_LOCATION_NAME_TRUNCATED | DVM_LOCATION_PATH_TRUNCATED)))
    {
        return 0;
    }
    if (location->flags & DVM_LOCATION_VALID)
    {
        return location->sequence && location->nprog > 0 && location->next_pc >= 0 &&
               location->next_pc <= location->nprog;
    }
    return !location->flags && !location->next_pc && !location->nprog && !location->name_bytes && !location->path_bytes;
}


/* EN: Borrow rooted frame; peek scalar header without consuming the ticket.
 * Retry only guarded publication. Failed/pending projection has zero payload.
 * RU: Заимствовать rooted frame; читать scalar header без consume ticket.
 * Повторять только guarded publication. Failed/pending projection даёт нули. */
void Processctl_residentlocation(void *frame)
{
    F_Processctl_residentlocation call;
    LONG result[7] = {DVM_DIAG_INVALID, 0, 0, 0, 0, 0, 0}; /* Status and copied location geometry. */
    DVM_DiagnosticSample sample;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Processctl_residentlocation, ret), sizeof(*call.ret));
    if (sizeof(result) != sizeof(*call.ret))
    {
        error(exBounds);
    }
    if (call.id > 0 && call.ticket > 0)
    {
        result[0] = dvm_resident_debug_peek((unsigned long)call.id, currun()->pid, (unsigned long)call.ticket, &sample);
        if (result[0] == DVM_DIAG_READY)
        {
            if (!valid_debug_location(&sample.location))
            {
                result[0] = DVM_DIAG_INVALID;
            }
            else
            {
                result[1] = (LONG)sample.location.sequence;
                result[2] = sample.location.next_pc;
                result[3] = sample.location.nprog;
                result[4] = sample.location.name_bytes;
                result[5] = sample.location.path_bytes;
                result[6] = sample.location.flags;
            }
        }
    }
    dvm_sys_scalar_result(result, call.ret, sizeof(result));
}


/* EN: Read one byte from an immutable ticket snapshot; validate part/index
 * before any native array access. Non-Ready output is zero, credit stays live.
 * RU: Читать byte из неизменяемого ticket snapshot; проверять part/index до
 * native array access. Non-Ready output нулевой; credit сохраняется. */
void Processctl_residentlocationbyte(void *frame)
{
    F_Processctl_residentlocationbyte call;
    LONG result[2] = {DVM_DIAG_INVALID, 0}; /* Status and unsigned UTF-8 byte. */
    DVM_DiagnosticSample sample;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Processctl_residentlocationbyte, ret), sizeof(*call.ret));
    if (sizeof(result) != sizeof(*call.ret))
    {
        error(exBounds);
    }
    if (call.id > 0 && call.ticket > 0 && call.part >= 0 && call.part <= 1 && call.index >= 0)
    {
        result[0] = dvm_resident_debug_peek((unsigned long)call.id, currun()->pid, (unsigned long)call.ticket, &sample);
        if (result[0] == DVM_DIAG_READY)
        {
            int length = call.part ? sample.location.path_bytes : sample.location.name_bytes;
            if (!valid_debug_location(&sample.location) || call.index >= length)
            {
                result[0] = DVM_DIAG_INVALID;
            }
            else
            {
                result[1] =
                    (unsigned char)(call.part ? sample.location.path[call.index] : sample.location.name[call.index]);
            }
        }
    }
    dvm_sys_scalar_result(result, call.ret, sizeof(result));
}
