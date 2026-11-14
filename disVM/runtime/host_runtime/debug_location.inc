/* Copyright (c) 2026 bhgv
 * EN: Capture this owner's live Dis boundary before releasing its managed claim.
 * RU: Снять живую Dis boundary своего owner до освобождения managed claim.
 */


/* [GC:] EN: Read only this thread's current registers and rooted module while ACTIVE.
 * Publish a private copied record; no allocation, callbacks or foreign REG access.
 * RU: Читать только текущие registers своего потока и rooted module в ACTIVE.
 * Публиковать private copy; без allocation, callbacks либо foreign REG access. */
static void runtime_capture_debug_location(void)
{
    DVM_DiagnosticLocation location;
    DVM_DiagnosticStack stack;
    dvm_diagnostic_location_capture(&location, DVM_R.M, DVM_R.PC);
    dvm_diagnostic_stack_capture(&stack, &location, DVM_R.M, DVM_R.FP);
    dvm_resident_location_publish(&location);
    dvm_resident_stack_publish(&stack);
}
