/* Copyright (c) 2026 bhgv
 * EN: Permanent resident dispatch, errors and process retirement; included once by main.c.
 * RU: Исполнение постоянных процессов, ошибки и завершение; однократно подключается из main.c.
 */


/* EN: Execute on the permanent resident owner. Scheduler metadata has its own
 * locks; this is not exclusive mutator admission. Return a managed status or -1
 * after retiring a failed claim. Live registers stay on this OS thread; saved
 * process registers are publications, not the source for resuming an owner.
 * RU: Выполнить работу на постоянном владельце процесса. Данные планировщика
 * защищены собственными замками, а не эксклюзивным допуском mutator. Вернуть
 * статус либо -1 после ошибки. Рабочие регистры остаются на этом OS-потоке;
 * сохранённые регистры служат публикацией и не используются для возобновления.
 */
static int runtime_dispatch_turn(void)
{
    DVM_NativeContext *context = dvm_native_context_current();
    Prog *process;
    int cancelled = gc_worker && dispatch_guest_cancellations();

    if (native_spawn)
    {
        Prog *guest = dvm_managed_workers_guest();
        guest = guest ? guest : initial_guest;
        cancelled |= consume_channel_message(guest);
        cancelled |= consume_child_reply(guest);
        dvm_managed_workers_finish_reply(guest);
        cancelled |= consume_process_wait_requests(guest);
        fvm_port_sync_lock(channel_queue_mutex);
        process = dvm_runqueue_select_guest(guest);
        fvm_port_sync_unlock(channel_queue_mutex);
    }
    else
    {
        process = dvm_runqueue_head();
    }
    if (!process)
    {
        return cancelled ? DVM_MANAGED_TURN_MAINTENANCE : DVM_MANAGED_TURN_IDLE;
    }
    if (!native_spawn && dvm_native_execution()->progress_epoch != progress_epoch)
    {
        dvm_native_execution()->idle_slices = 0;
        dvm_native_execution()->progress_epoch = progress_epoch;
    }
    /* EN: The legacy slice watchdog does not bound a resident's lifetime or
     * contention retries. Explicit instruction limits are checked separately.
     * RU: Счётчик старых тактов не ограничивает жизнь закреплённого потока
     * или ожидание блокировки. Явный лимит инструкций проверяется отдельно. */
    if (!native_spawn && !dvm_runtime_instruction_limit && ++dvm_native_execution()->idle_slices > 100000)
    {
        fprintf(stderr, "disvm: execution budget exceeded\n");
        return -1;
    }
    if (!dvm_runqueue_claim(process))
    {
        error("cannot publish active Dis process in TLS owner");
    }
    if (native_spawn && __atomic_exchange_n(&((DVM_RuntimeProcess *)process)->table_cancel_pc, 0, __ATOMIC_ACQUIRE))
    {
        if (dvm_native_execution()->resident_registers_valid)
        {
            DVM_R.PC++;
            process->R.PC = DVM_R.PC;
        }
        else
        {
            process->R.PC++;
        }
    }
    up->prog = process;
    if (!dvm_process_environment_bind(process))
    {
        error("invalid managed process environment");
    }
    dvm_native_execution()->instruction_boundary_error = 0;
    if (waserror())
    {
        dvm_native_execution()->resident_registers_valid = 0;
        dvm_native_execution()->arithmetic_retry_opcode = 0;
        ((DVM_RuntimeProcess *)process)->arithmetic_retries = 0;
        process->R = DVM_R;
        dvm_native_context_set_process(context, nil);
        dvm_bigtable_abort(process);
        if ((__atomic_load_n(&process->flags, __ATOMIC_ACQUIRE) & Pkilled) &&
            (native_spawn ? dvm_runqueue_is_ready(process) : dvm_runqueue_head() == process))
        {
            owner_error()[0] = 0;
        }
        int handled = 0;
        if (owner_error()[0] && !dvm_native_execution()->instruction_boundary_error)
        {
            if (!dvm_native_context_set_process(context, process))
            {
                abort();
            }
            handled = handler(owner_error());
            if (!dvm_native_context_set_process(context, nil))
            {
                abort();
            }
        }
        if (handled)
        { /* The handler already installed its target and frame in TLS. */
            Prog *resident = native_spawn ? dvm_managed_workers_guest() : nil;
            dvm_native_execution()->resident_registers_valid =
                native_spawn && process == (resident ? resident : initial_guest);
            owner_error()[0] = 0;
            if (!dvm_runqueue_finish(process))
            {
                error("cannot finish handled dispatch");
            }
            dvm_process_environment_release(process);
            up->prog = nil;
            return 1;
        }
        if (owner_error()[0])
        {
            const char *path;
            long pc;
            runtime_error_location(&path, &pc);
            fprintf(stderr, "Dis process %d module %s pc %ld: %s\n", process->pid, path, pc, owner_error());
            if (!dvm_runqueue_finish(process))
            {
                error("cannot finish failed dispatch");
            }
            dvm_process_environment_release(process);
            inspect_release(); /* [GC:] The per-process guard terminates here without reaching the outer guard. */
            up->prog = nil;
            return -1;
        }
        if (dvm_runqueue_owner(process) || ((DVM_RuntimeProcess *)process)->channel_claimed ||
            !dvm_runqueue_begin_retire(process))
        {
            error("cannot retire a process with a live native or channel claim");
        }
        if (native_spawn)
        {
            __atomic_fetch_add(&resident_retirements, 1, __ATOMIC_RELEASE);
        }
        if (!dvm_runqueue_finish(process))
        {
            error("cannot finish exiting dispatch");
        }
        dvm_process_environment_release(process);
        delrun(Pexiting);
        fvm_port_sync_lock(channel_queue_mutex);
        int removed = dvm_runtime_process_root_remove(process);
        fvm_port_sync_unlock(channel_queue_mutex);
        if (!removed)
        {
            error("retiring process lost its GC root");
        }
        /* Withdraw this root set before running any frame or module destructor. */
        dvm_native_context_set_process(context, nil);
        destroystack(&process->R);
        destroy(process->R.M);
        up->prog = nil;
        destroy(process->exval);
        free(process->exstr);
        {
            DVM_RuntimeProcess *retired = (DVM_RuntimeProcess *)process;
            int parent_pid = retired->exit_notify_parent ? retired->parent_pid : 0;
            int child_pid = process->pid;
            if (process == initial_guest)
            {
                initial_guest = NULL;
            }
            if (native_spawn)
            {
                dvm_managed_workers_guest_retired(child_pid);
            }
            drop_guest_messages(process);
            free(process);
            __atomic_fetch_sub(&live, 1, __ATOMIC_RELAXED);
            publish_child_exit(parent_pid, child_pid);
            if (native_spawn)
            {
                __atomic_fetch_sub(&resident_retirements, 1, __ATOMIC_RELEASE);
                dvm_managed_workers_wake();
            }
        }
    }
    else
    {
        if (__atomic_load_n(&process->flags, __ATOMIC_ACQUIRE) & Pkilled)
        { /* A resident with live registers already owns its unwind state. */
            if (!dvm_native_execution()->resident_registers_valid)
            {
                DVM_R = process->R;
            }
            dvm_runtime_termination_checkpoint(process);
        }
        xec(process);
        dvm_runtime_termination_checkpoint(process);
        /* The controller's initial Prog is also bound to its thread.
         * Channel and child-wait completions keep the live TLS registers. */
        int valid_registers = 0;
        if (native_spawn)
        {
            fvm_port_sync_lock(channel_queue_mutex);
            valid_registers = process->state == Pready || hosted_channel_waiter(process) ||
                              process->state == DVM_CHILD_EXIT_WAIT || process->state == DVM_TABLE_WAIT;
            fvm_port_sync_unlock(channel_queue_mutex);
        }
        dvm_native_execution()->resident_registers_valid = valid_registers;
        if (!dvm_runqueue_finish(process))
        {
            error("cannot finish completed dispatch");
        }
        dvm_process_environment_release(process);
        dvm_native_context_set_process(context, nil);
        up->prog = nil;
        poperror();
        if ((__atomic_load_n(&process->flags, __ATOMIC_ACQUIRE) & Pkilled) &&
            (native_spawn ? !dvm_runqueue_is_ready(process) : dvm_runqueue_head() != process))
        {
            wake_for_termination(process);
        }
        if (!native_spawn && dvm_runqueue_head() == process)
        {
            if (!dvm_runqueue_pop_expected(process, Pready))
            {
                error("invalid runnable process rotation");
            }
            addrun(process);
        }
    }
    if (dvm_runtime_process_roots() && gc_worker && !dvm_runtime_gc_request())
    {
        error("native collector failed");
    }
    if (dvm_runtime_process_roots() && !gc_worker)
    {
        rungc(dvm_runtime_process_roots());
    }
    return 1;
}
