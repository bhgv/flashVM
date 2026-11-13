/* Copyright (c) 2026 bhgv
 * EN: Resident mailbox replies and cancellation retry admission; included once by main.c.
 * RU: Ответы mailbox процесса и допуск повторов отмены; однократно подключается из main.c.
 */


/* Bind relative imports to the entry module's directory without changing caller cwd.
 * path is borrowed argv storage. Returns its basename for the initial logical module load. */
static char *entry_resource(char *path)
{
    char *separator = strrchr(path, '/');
    size_t length;
    if (strlen(path) >= 4096)
    {
        error("entry module path is too long");
    }
    if (!separator)
    {
        module_directory = strdup(".");
        if (!module_directory)
        {
            error(exNomem);
        }
        return path;
    }
    length = separator - path;
    if (!length)
    {
        length = 1;
    }
    module_directory = malloc(length + 1);
    if (!module_directory)
    {
        error(exNomem);
    }
    memcpy(module_directory, path, length);
    module_directory[length] = 0;
    return separator + 1;
}


/* EN: Let the dedicated owner complete guest's posted channel wait request.
 * RU: Завершить ожидание guest по сообщению на его потоке. */
static int consume_channel_message(Prog *guest)
{
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)guest;
    if (!process || !process->messages || !dvm_messages_pending(process->messages, 0, 0))
    {
        return 0;
    }
    return cancel_parked_guest(guest);
}


/* EN: Apply a borrowed reply under channel_queue_mutex only when both child
 * identity and operation ID match this wait. Return whether the parent woke;
 * stale replies leave the current continuation untouched. Storage stays with caller.
 * RU: Применить заимствованный ответ под channel_queue_mutex только при совпадении
 * ребёнка и ID операции. Вернуть признак пробуждения; старый ответ не меняет текущее
 * продолжение. Память остаётся у caller. */
static int apply_child_reply(Prog *guest, DVM_RuntimeProcess *process, DVM_OwnerMessage *reply)
{
    if (!reply || dvm_runqueue_observed_state(guest) != DVM_CHILD_EXIT_WAIT ||
        reply->sender != (uintptr_t)process->exit_wait_pid)
    {
        return 0;
    }
    if (process->cancel_waiting && reply->kind == DVM_PROG_MESSAGE_CANCEL_RESULT &&
        reply->request == process->cancel_wait_request)
    {
        process->cancel_ready_pid = (int)reply->sender;
        process->cancel_ready_result = (int)reply->value;
    }
    else if (!process->cancel_waiting && reply->kind == DVM_PROG_MESSAGE_CHILD_RETIRED &&
             reply->request == (uint64_t)process->exit_wait_pid)
    {
        process->exit_ready_pid = (int)reply->request;
    }
    else
    {
        return 0;
    }
    return wake_child_exit_wait(guest);
}


/* EN: Owner takes at most one staged/queued reply for borrowed guest. Apply or
 * discard it, freeing owned cancellation storage; retirement nodes stay embedded.
 * The transit counter covers removal through disposal. Return whether guest woke.
 * RU: Владелец забирает не более одного извлечённого/очередного ответа guest.
 * Применить либо отбросить, освободив память отмены; узлы retirement встроены.
 * Счётчик transit покрывает извлечение и удаление. Вернуть признак пробуждения. */
static int consume_child_reply(Prog *guest)
{
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)guest;
    if (!process || !process->messages ||
        (!dvm_messages_pending(process->messages, 1, 0) && !dvm_managed_workers_reply_in_transit(guest)))
    {
        return 0;
    }
    __atomic_fetch_add(&resident_wait_deliveries, 1, __ATOMIC_ACQ_REL);
    DVM_OwnerMessage *reply = dvm_managed_workers_take_reply(guest);
    if (!reply)
    {
        reply = dvm_messages_take(process->messages, 1, 0);
    }
    fvm_port_sync_lock(channel_queue_mutex);
    int delivered = apply_child_reply(guest, process, reply);
    if (reply && reply->kind == DVM_PROG_MESSAGE_CANCEL_RESULT)
    {
        free(reply);
    }
    fvm_port_sync_unlock(channel_queue_mutex);
    __atomic_fetch_sub(&resident_wait_deliveries, 1, __ATOMIC_RELEASE);
    return delivered;
}


/* EN: Consume guest wait requests on its permanent owner. First check the
 * mailbox, then acquire the atomic state publication. A table wake uses a
 * different mutex from channel publication, so the raw state is not safe here.
 * Return whether any request completed; guest remains independently alive.
 * RU: Принять запросы guest на постоянном владельце. После проверки mailbox
 * прочитать атомарную публикацию state: пробуждение таблицы использует другой
 * mutex. Вернуть признак завершения; guest удерживается отдельно. */
static int consume_process_wait_requests(Prog *guest)
{
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)guest;
    if (!process || !process->messages || !dvm_messages_pending(process->messages, 0, 0))
    {
        return 0;
    }
    int state = dvm_runqueue_observed_state(guest);
    if (state != DVM_CHILD_EXIT_WAIT && state != DVM_TABLE_WAIT && state != Pready)
    {
        return 0;
    }
    __atomic_fetch_add(&resident_wait_deliveries, 1, __ATOMIC_ACQ_REL);
    int cancelled = 0;
    DVM_OwnerMessage *message;
    while ((message = dvm_messages_take(process->messages, 0, 0)))
    {
        if (message->kind == DVM_PROG_MESSAGE_CHANNEL_EXIT)
        {
            cancelled |= wake_for_termination(guest);
        }
        else if (message->kind == DVM_PROG_MESSAGE_CANCEL_WAIT || message->kind == DVM_PROG_MESSAGE_CHANNEL_CANCEL)
        {
            process->cancel_delivering = dvm_bigtable_cancel_wait(guest) || cancel_child_exit_wait(guest);
            cancelled |= process->cancel_delivering;
            dvm_runtime_wait_request_finished(guest, message);
            process->cancel_delivering = 0;
        }
    }
    __atomic_fetch_sub(&resident_wait_deliveries, 1, __ATOMIC_RELEASE);
    return cancelled;
}


/* EN: Post the cancelled table call's PC adjustment to its TLS owner.
 * RU: Передать поправку PC отменённого вызова владельцу регистров. */


/* Serialize retry admission with publication of a cancellation request. */
int dvm_runtime_table_retry_begin(Prog *process)
{
    DVM_RuntimeProcess *resident = (DVM_RuntimeProcess *)process;
    if (!native_spawn || !__atomic_load_n(&resident->table_retry_pending, __ATOMIC_ACQUIRE))
    {
        return 1;
    }


    fvm_port_sync_lock(channel_queue_mutex);
    int pending = dvm_messages_pending(resident->messages, 0, DVM_PROG_MESSAGE_CANCEL_WAIT) ||
                  dvm_messages_pending(resident->messages, 0, DVM_PROG_MESSAGE_CHANNEL_CANCEL) ||
                  dvm_messages_pending(resident->messages, 0, DVM_PROG_MESSAGE_CHANNEL_EXIT);
    if (!pending)
    {
        __atomic_store_n(&resident->table_retry_pending, 0, __ATOMIC_RELEASE);
    }
    fvm_port_sync_unlock(channel_queue_mutex);
    return !pending;
}

void dvm_runtime_table_retry_pending(Prog *process, int pending)
{
    __atomic_store_n(&((DVM_RuntimeProcess *)process)->table_retry_pending, pending, __ATOMIC_RELEASE);
}

void dvm_runtime_table_cancel_pc(Prog *process)
{
    if (native_spawn)
    {
        DVM_RuntimeProcess *resident = (DVM_RuntimeProcess *)process;
        __atomic_store_n(&resident->table_cancel_pc, 1, __ATOMIC_RELEASE);
    }
    else
    {
        process->R.PC++;
    }
}
