/* Copyright (c) 2026 bhgv
 * EN: Permanent resident thread execution and teardown.
 * RU: Исполнение и завершение закреплённого потока процесса.
 */


/* EN: Run one permanently bound guest using borrowed argument slot through registrar join. Own
 * TLS/context and live registers; publish done under service.sync after teardown. Disable
 * cancellation; latch setup/turn/retirement failures, never migrate Prog or restore registers from
 * Prog.R.
 * RU: Исполнять один закреплённый процесс со слотом argument, заимствованным до join. Владеть
 * TLS/контекстом и живыми регистрами; после очистки опубликовать done под service.sync. Запретить
 * отмену, фиксировать ошибки старта/хода/завершения; не переносить Prog и не восстанавливать
 * регистры из Prog.R.
 */
static void execute(void *argument)
{
    ManagedWorker *worker = argument;
    if (worker->guest_pid)
    {
        fvm_port_sync_lock(worker->wake);
        while (!worker->startup_committed)
        {
            fvm_port_sync_wait(worker->wake);
        }
        fvm_port_sync_unlock(worker->wake);
    }
    int cancellation = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    DVM_NativeContext *context = cancellation ? NULL : dvm_native_process_create(NULL);
    int attached = 0;
    if (!context || !dvm_native_context_bind(context))
    {
        fail(cancellation ? "cannot disable executor thread cancellation" : "cannot bind executor");
        if (context)
        {
            dvm_native_context_destroy(context);
        }
        fvm_port_sync_lock(service.sync);
        unregister_dedicated_wait(worker);
        publish_resident_phase(worker, DVM_RESIDENT_DONE);
        worker->done = 1;
        service.generation++;
        fvm_port_sync_signal_all(service.sync);
        fvm_port_sync_unlock(service.sync);
        return;
    }
    if (worker->guest_pid && !dvm_runqueue_publish_startup(worker->guest))
    {
        fail("invalid dedicated resident publication");
    }
    while (!stopping())
    {
        if (dvm_runtime_gc_attach())
        {
            attached = 1;
            break;
        }
        fvm_port_thread_sleep_us(50);
    }
    if (attached)
    {
        if (worker->guest_pid)
        {
            current_guest = worker;
            if (service.diagnostics)
            {
                fprintf(
                    stderr, "NATIVEGUEST start pid=%d thread=%lu\n", worker->guest_pid, (unsigned long)pthread_self());
            }
        }
        fvm_port_sync_lock(service.sync);
        worker->identity = (uintptr)context;
        fvm_port_sync_signal_all(service.sync);
        fvm_port_sync_unlock(service.sync);
        if (waserror())
        {
            DVM_HeapOwnerStats failed_owner;
            fail(up->env->errstr);
            /* A turn-protocol fault cannot safely abandon a live C continuation.
             * Ordinary managed errors retire their claim before returning. */
            if (dvm_native_context_turn(context) || dvm_native_context_process(context) ||
                !dvm_native_owner_statistics(dvm_native_owner_current(), &failed_owner) ||
                failed_owner.phase != DVM_HEAP_ACTIVE)
            {
                fprintf(stderr, "disvm: worker retained an unfinished turn\n");
                exit(1);
            }
            up->prog = nil;
            up->env = &up->defenv;
        }
        else
        {
            while (!stopping() && !worker->guest_retired)
            {
                publish_resident_phase(worker, DVM_RESIDENT_DISPATCH);
                int progress = service.turn();
                publish_resident_phase(worker, DVM_RESIDENT_SCHEDULER);
                if (progress < 0)
                {
                    fail("managed turn failed");
                    break;
                }
                if (progress == DVM_MANAGED_TURN_GUEST)
                {
                    worker->turns++;
                }
                /* Stop owns the registry through join. A resumed native call
                 * may finish its current turn, but must not reap/mutate links. */
                if (stopping() || worker->guest_retired)
                {
                    break;
                }
                worker->handoffs++;
                if (!yield_empty(!progress, 0, NULL))
                {
                    if (!stopping())
                    {
                        error("cannot yield helper scheduler turn");
                    }
                    break;
                }
            }
            poperror();
        }
        publish_resident_phase(worker, DVM_RESIDENT_TEARDOWN);
        if (worker->guest_retired && service.diagnostics)
        {
            fprintf(
                stderr, "NATIVEGUEST retired pid=%d thread=%lu\n", worker->guest_pid, (unsigned long)pthread_self());
        }
        /* EN: Stop may arrive after a native reply was staged but before the
         * next dispatch. This owner still owns that node outside the endpoint.
         * RU: Stop может прийти после извлечения ответа до следующего dispatch.
         * Этот владелец ещё хранит узел вне endpoint. */
        if (worker->staged_reply && (worker->staged_reply->kind == DVM_PROG_MESSAGE_CANCEL_RESULT ||
                                     worker->staged_reply->kind == DVM_PROG_MESSAGE_CHILD_RETIRED))
        {
            free(worker->staged_reply);
        }
        worker->staged_reply = NULL;
        __atomic_store_n(&worker->reply_in_transit, 0, __ATOMIC_RELEASE);
        current_guest = NULL;
        fvm_port_sync_lock(service.sync);
        worker->identity = 0;
        fvm_port_sync_unlock(service.sync);
        if (!dvm_native_owner_statistics(dvm_native_owner_current(), &worker->owner))
        {
            fail("cannot snapshot executor ownership");
        }
        else if (worker->owner.phase == DVM_HEAP_ACTIVE)
        {
            fvm_port_sync_lock(service.sync);
            service.live--;
            fvm_port_sync_unlock(service.sync);
        }
        /* A dedicated worker stopped while IDLE leaves its slot for shutdown join. */
        while (!dvm_runtime_gc_detach())
        {
            fvm_port_thread_sleep_us(50);
        }
    }
    publish_resident_phase(worker, DVM_RESIDENT_TEARDOWN);
    if (!dvm_native_context_unbind(context) || !dvm_native_context_destroy(context))
    {
        fail("executor retirement failed");
    }
    fvm_port_sync_lock(service.sync);
    unregister_dedicated_wait(worker);
    publish_resident_phase(worker, DVM_RESIDENT_DONE);
    worker->done = 1;
    service.generation++;
    fvm_port_sync_signal_all(service.sync);
    fvm_port_sync_unlock(service.sync);
}
