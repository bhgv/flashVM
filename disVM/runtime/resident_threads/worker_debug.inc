/* Copyright (c) 2026 bhgv
 * EN: Registry-pinned diagnostic access and permanent-owner servicing.
 * RU: Registry-pinned diagnostic access и обслуживание permanent owner.
 */


/* EN: Answer at most two copied probes on this worker's permanent native owner.
 * Read native metadata only; no Prog/REG access, heap callback or guest execution.
 * RU: Ответить максимум на два copied probes на permanent native owner worker.
 * Читать native metadata; без Prog/REG, heap callback либо guest execution. */
static void service_debug_mailbox(ManagedWorker *worker)
{
    DVM_DiagnosticSample sample = {0};
    if (!dvm_diagnostic_pending(worker->debug))
    {
        return;
    }
    if (current_guest != worker || worker->identity != (uintptr)dvm_native_context_current())
    {
        fail("diagnostic mailbox serviced by foreign owner");
        return;
    }
    sample.location = worker->location;
    sample.owner_pid = worker->guest_pid;
    sample.turns = __atomic_load_n(&worker->observed_turns, __ATOMIC_ACQUIRE);
    sample.dispatches = __atomic_load_n(&worker->observed_dispatches, __ATOMIC_ACQUIRE);
    sample.waits = __atomic_load_n(&worker->observed_waits, __ATOMIC_ACQUIRE);
    dvm_diagnostic_answer(worker->debug, &sample, 2);
}


/* EN: Borrow registry guard to locate a positive exact lifetime ID. No Prog read.
 * RU: Под registry guard найти positive exact lifetime ID. Без Prog read. */
static ManagedWorker *find_debug_worker(unsigned long id)
{
    ManagedWorker *worker;
    for (worker = service.helpers; worker; worker = worker->next)
    {
        if (worker->id == id)
        {
            return worker;
        }
    }
    return NULL;
}


/* EN: Copy a request into an independently bounded mailbox; requester is the
 * original live caller PID. Registry pins box; signal only its owner's condition.
 * RU: Копировать request в отдельный bounded mailbox; requester — PID живого caller.
 * Registry удерживает box; сигнализировать только условие его owner. */
int dvm_resident_debug_request(unsigned long id, int requester, int64_t cookie, unsigned long *ticket)
{
    ManagedWorker *worker;
    int result = DVM_DIAG_MISSING;
    if (!ticket)
    {
        return DVM_DIAG_INVALID;
    }
    *ticket = 0;
    if (!id || id > (unsigned long)LLONG_MAX || requester <= 0)
    {
        return DVM_DIAG_INVALID;
    }
    if (!service.sync)
    {
        return DVM_DIAG_MISSING;
    }
    fvm_port_sync_lock(service.sync);
    worker = find_debug_worker(id);
    if (worker)
    {
        result = dvm_diagnostic_request(worker->debug, requester, cookie, ticket);
        if (result == DVM_DIAG_READY)
        {
            fvm_port_sync_lock(worker->wake);
            fvm_port_sync_signal_all(worker->wake);
            fvm_port_sync_unlock(worker->wake);
        }
    }
    fvm_port_sync_unlock(service.sync);
    return result;
}


/* EN: Read original requester reply under exact registry pin; peek selects
 * nonconsuming copy. Pending/failure output is zero; no target pin escapes.
 * RU: Читать reply original requester под exact registry pin; peek выбирает
 * копию без consume. Pending/failure output нулевой; target pin не выходит. */
static int
read_debug_reply(unsigned long id, int requester, unsigned long ticket, DVM_DiagnosticSample *sample, int peek)
{
    ManagedWorker *worker;
    int result = DVM_DIAG_MISSING;
    if (!sample)
    {
        return DVM_DIAG_INVALID;
    }
    memset(sample, 0, sizeof(*sample));
    if (!id || id > (unsigned long)LLONG_MAX || !ticket || ticket > (unsigned long)LLONG_MAX || requester <= 0)
    {
        return DVM_DIAG_INVALID;
    }
    if (!service.sync)
    {
        return DVM_DIAG_MISSING;
    }
    fvm_port_sync_lock(service.sync);
    worker = find_debug_worker(id);
    if (worker)
    {
        result = peek ? dvm_diagnostic_peek(worker->debug, requester, ticket, sample)
                      : dvm_diagnostic_reply(worker->debug, requester, ticket, sample);
    }
    fvm_port_sync_unlock(service.sync);
    return result;
}


/* EN/RU: Consume this original requester's completed reply from an exact resident.
 * Поглотить complete reply original requester из exact resident. */
int dvm_resident_debug_reply(unsigned long id, int requester, unsigned long ticket, DVM_DiagnosticSample *sample)
{
    return read_debug_reply(id, requester, ticket, sample, 0);
}


/* EN/RU: Copy this original requester's reply while retaining its bounded credit.
 * Копировать reply original requester, сохранив bounded credit. */
int dvm_resident_debug_peek(unsigned long id, int requester, unsigned long ticket, DVM_DiagnosticSample *sample)
{
    return read_debug_reply(id, requester, ticket, sample, 1);
}


/* EN: Forget only caller-owned query; registry keeps slot/box live through return.
 * RU: Забыть только свой query; registry удерживает slot/box до возврата. */
int dvm_resident_debug_forget(unsigned long id, int requester, unsigned long ticket)
{
    ManagedWorker *worker;
    int result = DVM_DIAG_MISSING;
    if (!id || id > (unsigned long)LLONG_MAX || !ticket || ticket > (unsigned long)LLONG_MAX || requester <= 0)
    {
        return DVM_DIAG_INVALID;
    }
    if (!service.sync)
    {
        return DVM_DIAG_MISSING;
    }
    fvm_port_sync_lock(service.sync);
    worker = find_debug_worker(id);
    if (worker)
    {
        result = dvm_diagnostic_forget(worker->debug, requester, ticket);
    }
    fvm_port_sync_unlock(service.sync);
    return result;
}


/* EN: Retiring caller releases all its bounded credits before losing PID/lifetime.
 * Registry pins native boxes; no endpoint or managed pointer escapes the scan.
 * RU: Завершающийся caller освобождает credits до потери PID/lifetime. Registry
 * удерживает native boxes; endpoint и managed pointer не выходят из scan. */
static void retire_debug_requester(int pid)
{
    ManagedWorker *worker;
    if (!service.sync || !dvm_diagnostic_has_requests())
    {
        return;
    }
    fvm_port_sync_lock(service.sync);
    for (worker = service.helpers; worker; worker = worker->next)
    {
        dvm_diagnostic_forget_requester(worker->debug, pid);
    }
    fvm_port_sync_unlock(service.sync);
}
