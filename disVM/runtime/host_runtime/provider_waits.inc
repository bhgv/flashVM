/* Copyright (c) 2026 bhgv
 * EN: Keep native provider cancellation registered from FD admission through data completion.
 * RU: Сохранить native provider cancellation от FD admission до data completion.
 */


/* EN/RU: Owned only by the source native caller; ledger stores copied PID/native pins.
 * Принадлежит source native caller; ledger хранит copied PID/native pins. */
struct DVM_VfsWait
{
    NativeGuestWait wait; /* Linked native wait, no managed pointer. */
    DVM_Cancel *cancel; /* Owned per-operation cancellation source. */
    DVM_CancelScope scope; /* Same-thread synchronous native propagation. */
};


/* EN/RU: See native_wait.h; unlink and clear TLS before managed reentry can unwind.
 * См. native_wait.h; unlink и очистить TLS до managed reentry/unwind. */
int dvm_vfs_wait_end(DVM_VfsWait *wait)
{
    if (!wait)
    {
        return 0;
    }
    int cancelled = native_guest_wait_end(&wait->wait, 0) < 0 || dvm_cancel_requested(wait->cancel);
    if (!dvm_cancel_leave(&wait->scope))
    {
        fprintf(stderr, "disvm: provider cancellation scope ownership lost\n");
        abort();
    }
    dvm_cancel_release(wait->cancel);
    free(wait);
    return cancelled;
}


/* EN/RU: See native_wait.h; all failure paths remove reservation and native ownership.
 * См. native_wait.h; каждый failure удаляет reservation и native ownership. */
int dvm_vfs_gate_enter(DVM_FileGate *gate, DVM_FileTicket *ticket, DVM_VfsWait **output)
{
    *output = NULL;
    dvm_file_gate_reserve(gate, ticket);
    DVM_VfsWait *wait = calloc(1, sizeof(*wait));
    if (wait)
    {
        wait->cancel = dvm_cancel_create();
    }
    if (!wait || !wait->cancel || !dvm_cancel_enter(&wait->scope, wait->cancel))
    {
        dvm_file_gate_cancel(gate, ticket);
        if (wait)
        {
            dvm_cancel_release(wait->cancel);
            free(wait);
        }
        return -1;
    }
    native_guest_wait_begin_operation(&wait->wait, gate, ticket, -1, wait->cancel);
    int released = native_guest_wait_try_leave(&wait->wait);
    if (released < 0)
    {
        dvm_file_gate_cancel(gate, ticket);
        dvm_vfs_wait_end(wait);
        return released;
    }
    int admitted = released ? dvm_file_gate_enter(gate, ticket) : dvm_file_gate_enter_now(gate, ticket);
    if (!admitted)
    {
        dvm_vfs_wait_end(wait);
        dvm_runtime_gc_native_enter(released);
        return released ? -2 : -1;
    }
    *output = wait;
    return released;
}
