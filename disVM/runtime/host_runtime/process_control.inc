/* Copyright (c) 2026 bhgv
 * EN: Child-process waits, replies and termination checkpoints; included once by main.c.
 * RU: Ожидания детей, ответы и проверки завершения; однократно подключается из main.c.
 */


/* EN: Consume an owned cancellation request, reusing its allocation as the reply.
 * guest is held by the caller. Registration is protected by the queue mutex;
 * the parent validates the operation ID when consuming. Discarded requests are
 * freed here; successful send transfers ownership, so never touch them again.
 * Other message kinds remain borrowed embedded nodes and are unchanged.
 * RU: Принять владеемый запрос отмены, используя его память для ответа.
 * Caller удерживает guest. Mutex очереди защищает регистрацию; родитель проверяет
 * ID операции при чтении. Отброшенные запросы освободить здесь; после отправки
 * владение передано, узел больше не трогать. Иные виды — заимствованные встроенные
 * узлы, они остаются без изменений. */
void dvm_runtime_wait_request_finished(Prog *guest, DVM_OwnerMessage *message)
{
    if (message->kind != DVM_PROG_MESSAGE_CANCEL_WAIT)
    {
        return;
    }
    fvm_port_sync_lock(channel_queue_mutex);
    Prog *parent;
    for (parent = dvm_runtime_process_roots(); parent && (uintptr_t)parent->pid != message->sender;
         parent = parent->next)
    {
    }
    if (!parent)
    {
        fvm_port_sync_unlock(channel_queue_mutex);
        free(message);
        return;
    }
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)parent;
    if (!process->cancel_waiting || process->exit_wait_pid != guest->pid)
    {
        fvm_port_sync_unlock(channel_queue_mutex);
        free(message);
        return;
    }
    DVM_OwnerMessage *reply = message;
    reply->sender = (uintptr_t)guest->pid;
    reply->kind = DVM_PROG_MESSAGE_CANCEL_RESULT;
    reply->value = ((DVM_RuntimeProcess *)guest)->cancel_delivering ? Processctl_Cancelled : Processctl_NotWaiting;
    if (!dvm_messages_send(guest_messages, (uintptr_t)parent->pid, 1, reply))
    {
        abort();
    }
    dvm_managed_workers_wake_guest(parent);
    fvm_port_sync_unlock(channel_queue_mutex);
}


/* Requeue an exiting parent without disturbing the child's exit request. */
static int wake_child_exit_wait(Prog *process)
{
    if (dvm_runqueue_observed_state(process) != DVM_CHILD_EXIT_WAIT)
    {
        return 0;
    }
    ((DVM_RuntimeProcess *)process)->exit_wait_pid = 0;
    ((DVM_RuntimeProcess *)process)->cancel_waiting = 0;
    ((DVM_RuntimeProcess *)process)->cancel_wait_request = 0;
    addrun(process);
    return 1;
}


/* Remove a parent's interest without undoing the already requested exit. */
static int detach_child_exit_wait(Prog *process)
{
    Prog *child;

    if (dvm_runqueue_observed_state(process) != DVM_CHILD_EXIT_WAIT)
    {
        return 0;
    }
    if (native_spawn)
    {
        fvm_port_sync_lock(channel_queue_mutex);
    }
    child = find_child(process, ((DVM_RuntimeProcess *)process)->exit_wait_pid);
    if (child && !((DVM_RuntimeProcess *)process)->cancel_waiting)
    {
        ((DVM_RuntimeProcess *)child)->exit_notify_parent = 0;
    }
    int ready = wake_child_exit_wait(process);
    if (native_spawn)
    {
        fvm_port_sync_unlock(channel_queue_mutex);
    }
    return ready;
}


/* Attribute cancellation to the original MCALL, whose PC was rewound
 * before parking. The normal exception path owns prepared-frame cleanup. */
static int cancel_child_exit_wait(Prog *process)
{
    if (native_spawn)
    {
        Prog *resident = dvm_managed_workers_guest();
        if (process != (resident ? resident : initial_guest))
        {
            return 0;
        }
    }
    if ((__atomic_load_n(&process->flags, __ATOMIC_ACQUIRE) & Pkilled) || !detach_child_exit_wait(process))
    {
        return 0;
    }
    if (native_spawn && dvm_native_execution()->resident_registers_valid)
    {
        DVM_R.PC++;
        process->R.PC = DVM_R.PC;
    }
    else
    {
        process->R.PC++;
    }
    if (!process->kill)
    {
        process->kill = "child exit wait cancelled";
    }
    return 1;
}


/* Return a blocked child to the scheduler for its ordinary exit turn. */
static int wake_for_termination(Prog *child)
{
    return cancel_parked_guest(child) || dvm_bigtable_wake_for_exit(child) || detach_child_exit_wait(child);
}


/* Post termination to the child owner before it resumes execution. */
static void send_resident_exit(Prog *child)
{
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)child;
    __atomic_fetch_or(&child->flags, Pkilled, __ATOMIC_RELEASE);
    if (dvm_messages_send_once(guest_messages, (uintptr_t)child->pid, &process->channel_exit) == 1)
    {
        dvm_managed_workers_wake_guest(child);
    }
}


/* EN: Borrow child while channel_queue_mutex protects its registration. A live
 * dedicated owner receives exit through its mailbox in any execution/wait phase.
 * Read only the atomic lifetime seal; never inspect its owner-written Prog.state.
 * Return zero for NULL or already sealed retirement, without changing ownership.
 * RU: Заимствовать child под channel_queue_mutex, защищающим регистрацию. Живой
 * выделенный владелец получает выход через mailbox в любой фазе исполнения или
 * ожидания. Читать лишь атомарный lifetime seal, не чужой Prog.state. Ноль для
 * NULL или уже закрытого retirement без изменения владения. */
static int resident_can_exit(Prog *child)
{
    return child && dvm_runqueue_hold_count(child) >= 0;
}


/* Schedule termination for a runnable or supported blocked child. Never
 * retire a native stack on a foreign worker. The child unwinds it first. */
static int request_child_exit(Prog *child)
{
    if (!child)
    {
        return Processctl_Denied;
    }
    if (dvm_runqueue_observed_state(child) == Prelease)
    {
        __atomic_fetch_or(&child->flags, Pkilled, __ATOMIC_RELEASE);
        dvm_runtime_cancel_guest_wait(child->pid);
        return Processctl_Requested;
    }
    if (dvm_runqueue_observed_state(child) == Pready || wake_for_termination(child))
    {
        __atomic_fetch_or(&child->flags, Pkilled, __ATOMIC_RELEASE);
        return Processctl_Requested;
    }
    return Processctl_NotWaiting;
}


/* Resolve the direct child under the caller's managed turn before requesting exit. */
int dvm_runtime_kill_child(Prog *caller, int pid)
{
    if (!native_spawn)
    {
        return request_child_exit(find_child(caller, pid));
    }

    int result = Processctl_Denied;
    fvm_port_sync_lock(channel_queue_mutex);
    Prog *child = find_child(caller, pid);
    if (resident_can_exit(child))
    {
        send_resident_exit(child);
        cancel_registered_guest_wait(pid);
        result = Processctl_Requested;
    }
    else if (child)
    {
        result = Processctl_NotWaiting;
    }
    fvm_port_sync_unlock(channel_queue_mutex);
    return result;
}


/* EN: Publish the parent wait for a live or already retiring registered child.
 * The atomic lifetime seal precedes Pexiting. A sealed child needs only
 * retirement notification, not another exit. Registry membership admits the
 * wait; sample the lifetime seal only to avoid an unnecessary exit request.
 * Rechecking a live sample can race sealing and wrongly skip the wait.
 * Reserve this child's single native reply before parking; allocation failure
 * leaves caller runnable. A cancelled/rearmed wait can reuse an untransferred reserve.
 * RU: Опубликовать ожидание живого или уже завершающегося ребёнка из реестра.
 * Атомарный lifetime seal предшествует Pexiting. Для sealed нужна только
 * квитанция завершения, а не новый запрос. Наличие в реестре разрешает ожидание;
 * lifetime-снимок лишь исключает лишний запрос выхода. Повторная проверка может
 * пересечь sealing и ошибочно пропустить ожидание. Чужой state не читать.
 * Резервировать единственный ответ ребёнка до парковки; отказ памяти оставляет
 * caller runnable. Отозванное/повторное ожидание использует непереданный резерв. */
static int prepare_dedicated_child_wait(Prog *caller, int pid)
{
    fvm_port_sync_lock(channel_queue_mutex);
    Prog *child = find_child(caller, pid);
    if (!child)
    {
        fvm_port_sync_unlock(channel_queue_mutex);
        return 0;
    }
    DVM_RuntimeProcess *target = (DVM_RuntimeProcess *)child;
    if (!target->retirement_reply)
    {
        target->retirement_reply = calloc(1, sizeof(*target->retirement_reply));
        if (!target->retirement_reply)
        {
            fvm_port_sync_unlock(channel_queue_mutex);
            error(exNomem);
        }
    }
    int retiring = dvm_runqueue_hold_count(child) < 0;
    if (!dvm_runqueue_pop_expected(caller, DVM_CHILD_EXIT_WAIT))
    {
        fvm_port_sync_unlock(channel_queue_mutex);
        error("invalid runnable process removal");
    }
    ((DVM_RuntimeProcess *)caller)->exit_wait_pid = pid;
    DVM_R.PC--;
    DVM_R.IC = 1;
    DVM_R.t = 1;
    ((DVM_RuntimeProcess *)child)->exit_notify_parent = 1;
    if (!retiring)
    {
        send_resident_exit(child);
        cancel_registered_guest_wait(pid);
    }
    fvm_port_sync_unlock(channel_queue_mutex);
    return 1;
}


/* Request termination before committing MCALL. A successful request parks
 * the parent on its prepared frame until the child's exit publishes completion. */
int dvm_runtime_prepare_killwait(Prog *caller, int pid)
{
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)caller;
    Prog *child;

    if (process->exit_ready_pid == pid)
    {
        return 1;
    }
    if (native_spawn)
    {
        return !prepare_dedicated_child_wait(caller, pid);
    }
    child = find_child(caller, pid);
    if (request_child_exit(child) != Processctl_Requested)
    {
        return 1;
    }
    ((DVM_RuntimeProcess *)child)->exit_notify_parent = 1;
    process->exit_wait_pid = pid;
    DVM_R.PC--;
    DVM_R.IC = 1;
    DVM_R.t = 1;
    delrun(DVM_CHILD_EXIT_WAIT);
    return 0;
}


/* Match an exit completion by exact child identity, then consume it. */
int dvm_runtime_killwait_result(Prog *caller, int pid)
{
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)caller;

    if (process->exit_ready_pid == pid)
    {
        process->exit_ready_pid = 0;
        return Processctl_Retired;
    }
    if (native_spawn)
    {
        fvm_port_sync_lock(channel_queue_mutex);
    }
    int result = find_child(caller, pid) ? Processctl_NotWaiting : Processctl_Denied;
    if (native_spawn)
    {
        fvm_port_sync_unlock(channel_queue_mutex);
    }
    return result;
}


/* EN: Consume owned message reserved before killwait parking. Publish only to
 * the matching parent wait after all child storage is gone; otherwise free it.
 * Parent lookup and wait metadata use the registry guard, not a world stop.
 * parent_pid/child_pid are copied identities, message contains no managed roots.
 * Reference mode supplies NULL and wakes synchronously. No retirement allocation.
 * RU: Принять message, зарезервированный до парковки killwait. После удаления
 * ребёнка отправить лишь совпадающему ожиданию родителя, иначе освободить.
 * Поиск родителя и его wait защищены реестром, без остановки мира.
 * parent_pid/child_pid — копии ID, message не хранит managed-корней.
 * Reference-режим передаёт NULL и будит синхронно. При retirement память не выделяется. */
static void publish_child_exit(int parent_pid, int child_pid, DVM_OwnerMessage *message)
{
    if (!parent_pid)
    {
        free(message);
        return;
    }
    if (native_spawn)
    {
        fvm_port_sync_lock(channel_queue_mutex);
    }
    Prog *parent;
    for (parent = dvm_runtime_process_roots(); parent && parent->pid != parent_pid; parent = parent->next)
    {
    }
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)parent;
    if (process && !process->cancel_waiting && process->exit_wait_pid == child_pid)
    {
        if (process->messages)
        {
            if (!message)
            {
                abort();
            }
            DVM_OwnerMessage *reply = message;
            reply->sender = (uintptr_t)child_pid;
            reply->request = (uint64_t)child_pid;
            reply->kind = DVM_PROG_MESSAGE_CHILD_RETIRED;
            if (!dvm_messages_send(guest_messages, (uintptr_t)parent_pid, 1, reply))
            {
                abort();
            }
            message = NULL;
            dvm_managed_workers_wake_guest(parent);
        }
        else
        {
            process->exit_ready_pid = child_pid;
            wake_child_exit_wait(parent);
        }
    }
    if (native_spawn)
    {
        fvm_port_sync_unlock(channel_queue_mutex);
    }
    free(message);
}


/* End a requested guest only when its own instruction and native cleanup
 * are complete. The process guard then takes the ordinary exit path. */
void dvm_runtime_termination_checkpoint(Prog *process)
{
    if ((__atomic_load_n(&process->flags, __ATOMIC_ACQUIRE) & Pkilled) &&
        (native_spawn ? dvm_runqueue_is_ready(process) : dvm_runqueue_head() == process))
    {
        error("");
    }
}


/* Consume the temporary channel owner inside a cleanup checkpoint. */
static int release_refill_channel(void *argument)
{
    destroy(argument);
    return 1;
}
