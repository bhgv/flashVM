/* Copyright (c) 2026 bhgv
 * EN: Keep native provider cancellation registered through path or FD operation completion.
 * RU: Сохранить native provider cancellation до завершения path либо FD operation.
 */


/* EN/RU: Owned only by the source native caller; ledger stores copied PID/native pins.
 * Принадлежит source native caller; ledger хранит copied PID/native pins. */
struct DVM_VfsWait
{
    NativeGuestWait wait; /* Linked native wait, no managed pointer. */
    DVM_Cancel *cancel; /* Owned per-operation cancellation source. */
    DVM_CancelTap *parent; /* Optional active remote call forwarded into this path operation. */
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
    dvm_cancel_unsubscribe(wait->parent);
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


/* EN/RU: Build/register source-owned native scope while ACTIVE; borrowed optional gate/ticket.
 * Return owned wait/NULL without managed effects. Создать/register native scope
 * в ACTIVE; optional gate/ticket borrowed, свой wait/NULL, без managed effects. */
static DVM_VfsWait *vfs_wait_start(DVM_FileGate *gate, DVM_FileTicket *ticket)
{
    DVM_VfsWait *wait = calloc(1, sizeof(*wait));
    if (wait)
    {
        wait->cancel = dvm_cancel_create();
    }
    if (!wait || !wait->cancel || !dvm_cancel_enter(&wait->scope, wait->cancel))
    {
        if (wait)
        {
            dvm_cancel_release(wait->cancel);
            free(wait);
        }
        return NULL;
    }
    DVM_RuntimeProcess *owner = (DVM_RuntimeProcess *)currun();
    DVM_Cancel *parent = !gate && owner && !owner->remote_call_cancel_delivered ?
        owner->remote_call_cancel : NULL;
    if (parent && !(wait->parent = dvm_cancel_forward(parent, wait->cancel)))
    {
        dvm_cancel_leave(&wait->scope);
        dvm_cancel_release(wait->cancel);
        free(wait);
        return NULL;
    }
    native_guest_wait_begin_operation(&wait->wait, gate, ticket, -1, wait->cancel);
    return wait;
}


/* EN/RU: See native_wait.h; all failure paths remove reservation and native ownership.
 * См. native_wait.h; каждый failure удаляет reservation и native ownership. */
int dvm_vfs_gate_enter(DVM_FileGate *gate, DVM_FileTicket *ticket, DVM_VfsWait **output)
{
    *output = NULL;
    dvm_file_gate_reserve(gate, ticket);
    DVM_VfsWait *wait = vfs_wait_start(gate, ticket);
    if (!wait)
    {
        dvm_file_gate_cancel(gate, ticket);
        return -1;
    }
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


/* EN/RU: See native_wait.h; path selection is already retained and no FD queue is involved.
 * См. native_wait.h; path selection уже удержан, FD queue не используется. */
int dvm_vfs_path_enter(DVM_VfsWait **output)
{
    *output = NULL;
    DVM_VfsWait *wait = vfs_wait_start(NULL, NULL);
    if (!wait)
    {
        return -1;
    }
    int released = dvm_cancel_requested(wait->cancel) ? -2 : native_guest_wait_try_leave(&wait->wait);
    if (released < 0)
    {
        dvm_vfs_wait_end(wait);
        return released;
    }
    *output = wait;
    return released;
}
