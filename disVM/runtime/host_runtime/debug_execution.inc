/* Copyright (c) 2026 bhgv
 * EN: Owner-only debug stops preserve the original TLS/native continuation.
 * RU: Debug stops только owner сохраняют original TLS/native continuation.
 */


/* [GC:] EN: Park a claimed source via audited native release; no register reload.
 * A guarded/nonreleasable continuation refuses rather than waiting while ACTIVE.
 * RU: Припарковать claimed source через audited native release; без REG reload.
 * Guarded/nonreleasable continuation отказывает, не ожидая в ACTIVE. */
static int runtime_debug_park(void)
{
    int released = dvm_runtime_gc_native_try_leave();
    if (released != 1)
    {
        fprintf(stderr, "disvm: debug stop refused at unreleasable continuation\n");
        return 0;
    }
    dvm_runtime_gc_native_enter(released);
    return 1;
}


/* [GC:] EN: Publish exact pre-execution opcode, then service pause/termination.
 * Only called from checked C/JIT instruction admission; no PC-1 reconstruction.
 * RU: Публиковать точный pre-execution opcode, обслужить pause/termination.
 * Только checked C/JIT instruction admission; без PC-1 reconstruction. */
static void runtime_debug_instruction(void)
{
    DVM_DiagnosticLocation location;
    if (!dvm_debug_bind(currun()->pid))
    {
        error(exNomem);
    }
    dvm_diagnostic_location_capture(&location, DVM_R.M, DVM_R.PC);
    if ((location.flags & DVM_LOCATION_VALID) && location.next_pc < location.nprog)
    {
        dvm_debug_instruction(&DVM_R, DVM_R.PC, &location);
    }
    if (dvm_debug_requested())
    {
        runtime_debug_park();
    }
    dvm_debug_instruction_enter();
    if (dvm_debug_terminated())
    {
        __atomic_fetch_or(&currun()->flags, Pkilled, __ATOMIC_RELEASE);
        dvm_runtime_termination_checkpoint(currun());
    }
}


/* [GC:] EN: Stop before handler/unwind while keeping the source's original error,
 * exval, native call frame and TLS REG. Return1 only after actual retained stop.
 * RU: Остановиться до handler/unwind с original error/exval/native frame/TLS REG.
 * Вернуть1 только после настоящего retained stop. */
static int runtime_debug_fault(Prog *process)
{
    DVM_NativeContext *context = dvm_native_context_current();
    int retained;
    if (!dvm_debug_enabled || !owner_error()[0] || (__atomic_load_n(&process->flags, __ATOMIC_ACQUIRE) & Pkilled))
    {
        return 0;
    }
    if (!dvm_debug_bind(process->pid) || !dvm_native_context_set_process(context, process))
    {
        return 0;
    }
    dvm_debug_fault(owner_error());
    retained = runtime_debug_park();
    if (!retained)
    {
        dvm_debug_stop_refused();
    }
    dvm_native_context_set_process(context, NULL);
    if (dvm_debug_terminated())
    {
        owner_error()[0] = 0;
    }
    return retained;
}


/* [GC:] EN: A channel-blocked owner has no dispatch claim. Publish its own TLS
 * roots and leave rootless admission before the same debug reentry gate.
 * RU: Channel-blocked owner не имеет dispatch claim. Публиковать свои TLS roots
 * и выйти из rootless admission перед тем же debug reentry gate. */
static void runtime_debug_idle(Prog *process)
{
    if (!dvm_debug_enabled || !process || !dvm_native_execution()->resident_registers_valid ||
        dvm_runqueue_is_ready(process))
    {
        return;
    }
    if (!dvm_debug_bind(process->pid))
    {
        error(exNomem);
    }
    if (dvm_debug_requested())
    {
        process->R = DVM_R;
        if (!dvm_managed_workers_debug_leave())
        {
            error("cannot publish idle debug continuation");
        }
        dvm_debug_native_publish(process, &DVM_R);
        dvm_debug_native_gate(process);
        dvm_managed_workers_debug_enter();
    }
    if (dvm_debug_terminated())
    {
        __atomic_fetch_or(&process->flags, Pkilled, __ATOMIC_RELEASE);
        wake_for_termination(process);
    }
}
