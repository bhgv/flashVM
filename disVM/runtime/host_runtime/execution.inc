/* Copyright (c) 2026 bhgv
 * EN: Runtime startup, scheduler loop and joined shutdown; included once by main.c.
 * RU: Запуск runtime, цикл планировщика и завершение после join; однократно подключается из main.c.
 */


/* EN: Initialize the runtime and drive its controller on the bound native owner.
 * argument is a borrowed module path valid until join/return. Native guests run
 * on their permanent threads; their channel operations use the guarded paths.
 * Native collectors cannot stop the world. Return nonzero on startup/runtime
 * failure; the outer wrapper joins services before retiring owner TLS.
 * RU: Инициализировать runtime и вести контроллер на привязанном владельце.
 * argument — путь модуля до join/возврата. Гости исполняются на постоянных потоках,
 * каналы используют защищённые операции, native GC не останавливает мир.
 * При ошибке вернуть ненулевой код; внешняя обёртка делает join до удаления TLS. */
static int runtime_execute_inner(void *argument)
{
    DVM_NativeContext *context;
    Module *module;
    Prog *process;
    char *entry;
    context = dvm_native_process_create(nil);
    if (!context || !dvm_native_context_bind(context))
    {
        return 1;
    }
    if (pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL))
    {
        fprintf(stderr, "cannot initialize cooperative VM cancellation discipline\n");
        return 1;
    }
    up->env = &up->defenv;
    up->env->errstr = up->env->errbuf1;
    up->env->syserrstr = up->env->errbuf0;
    if (waserror())
    {
        inspect_release();
        fprintf(stderr, "disvm: %s\n", owner_error());
        return 1;
    }
    arena_init();
    opinit();
    sysmodinit();
    dvm_filesystem_initialize();
    loadermodinit();
    mathmodinit();
    dvm_bigtable_initialize();
    dvm_processctl_initialize();
    dvm_proxyhost_initialize();
    dvm_proxy_gc_initialize();
#ifdef DVM_RUNTIME_COMPILER
    dvm_compiler_initialize();
#endif
#ifdef DVM_RUNTIME_NATIVE_DEMO
    dvm_nativedemo_initialize();
#endif
    excinit();
    dvm_srv_initialize();
    srvmodinit();
    entry = entry_resource((char *)argument);
#ifdef DVM_RUNTIME_EDITOR
    if (!dvm_editor_start(module_directory))
    {
        dvm_editor_stop();
        error("cannot initialize graphical editor");
    }
#endif
    module = load(entry);
    if (!module)
    {
        error(owner_error());
    }
    if (native_spawn)
    {
        guest_messages = dvm_messages_create();
        if (!guest_messages)
        {
            error(exNomem);
        }
        dvm_runqueue_enable_direct();
    }
    process = start_module(module);
    initial_guest = process;
    if (inspect_requested)
    {
        inspect_retain(process);
    }
    if (gc_worker && !dvm_runtime_gc_start(gc_locked_poll))
    {
        error("cannot start native collector");
    }
    if (dvm_remote_managed_probe_requested)
    {
        dvm_remote_managed_probe();
    }
    if (native_spawn && !dvm_resident_threads_start(show_stats, runtime_dispatch_turn, dvm_runtime_cancel_native_waits))
    {
        error("cannot start resident thread registry");
    }
    if (gc_worker && !dvm_close_worker_start())
    {
        error("cannot start native close worker");
    }
    fvm_port_sync_lock(channel_queue_mutex);
    cancel_requests_open = gc_worker && !native_waits_stopping;
    fvm_port_sync_unlock(channel_queue_mutex);
    while ((dvm_debug_enabled && __atomic_load_n(&live, __ATOMIC_RELAXED)) || dvm_runqueue_ready() || (native_spawn && dvm_runqueue_dispatch_pending()) ||
           (native_spawn && dvm_runqueue_inflight())
#ifdef DVM_RUNTIME_EDITOR
           || (__atomic_load_n(&live, __ATOMIC_RELAXED) && dvm_editor_attached())
#endif
           || guest_cancel_pending_or_close())
    {

        if (gc_worker && !dvm_native_context_poll())
        {
            error("native collector safepoint failed");
        }
#ifdef DVM_RUNTIME_EDITOR
        if (dvm_editor_attached())
        {
            int progress = dvm_editor_poll();
            if (progress < 0)
            {
                error("graphical editor transport/presentation failure");
            }
            if (progress)
            {
                dvm_runtime_input_progress();
            }
        }
        if (!native_spawn && !dvm_runqueue_head() && !guest_cancel_pending())
        {
            fvm_port_thread_sleep_us(1000);
            continue;
        }
#endif
        {
            int progress = runtime_dispatch_turn();
            if (progress < 0)
            {
                poperror(); /* The dispatch retired its own guard; do not retain this returning outer jump frame. */
                return 1;
            }
            if (progress == DVM_MANAGED_TURN_GUEST)
            {
                dvm_managed_workers_record();
            }
#ifndef DVM_RUNTIME_EDITOR
            if (native_spawn && progress == DVM_MANAGED_TURN_IDLE &&
                (dvm_runqueue_ready() || dvm_runqueue_inflight() || dvm_runqueue_dispatch_pending() ||
                 (dvm_debug_enabled && __atomic_load_n(&live, __ATOMIC_RELAXED))))
            {
                if (!dvm_managed_workers_park(dedicated_controller_pending))
                {
                    error("dedicated guest wait failed");
                }
                continue;
            }
#endif
        }
        if (!dvm_managed_workers_yield(!native_spawn && !dvm_runqueue_head() && !guest_cancel_pending()))
        {
            error("managed scheduler worker failed");
        }
    }
    if (__atomic_load_n(&live, __ATOMIC_RELAXED))
    {
        error("deadlock: processes remain blocked on channels");
    }
#ifdef DVM_RUNTIME_EDITOR
    dvm_editor_stop();
#endif
    poperror();
    if (inspect_requested && !inspect_report())
    {
        fprintf(stderr, "disvm: inspection output failed\n");
        return 1;
    }
    return 0;
}


/* Join the collector on every managed normal/error return, before the editor
 * launcher retires mutator TLS. argument is borrowed through this call. */
static int runtime_execute(void *argument)
{
    int result = runtime_execute_inner(argument);
    dvm_debug_quiesce();
    dvm_runtime_cancel_native_waits();
    if (!dvm_managed_workers_stop(show_stats))
    {
        result = 1;
    }
    if (stdio_gates[1] == stdio_gates[2])
    {
        stdio_gates[2] = NULL;
    }
    for (int descriptor = 0; descriptor < 3; descriptor++)
    {
        if (!dvm_file_gate_destroy(stdio_gates[descriptor]))
        {
            fprintf(stderr, "disvm: standard pipe gate retains a native operation\n");
            result = 1;
        }
        else
        {
            stdio_gates[descriptor] = NULL;
        }
    }
    if (!dvm_runtime_gc_stop(show_stats))
    {
        result = 1;
    }
    dvm_close_worker_stop();
    if (guest_messages)
    {
        for (Prog *guest = dvm_runtime_process_roots(); guest; guest = guest->next)
        {
            drop_guest_messages(guest);
        }
        dvm_messages_destroy(guest_messages);
        guest_messages = NULL;
    }
    result |= dvm_debug_exit_status();
    dvm_debug_destroy();
    dvm_jit_signal_thread_stop();
    dvm_jit_signals_stop();
    runtime_statistics();
    dvm_queue_probe_finish();
    if (!result)
    {
        fprintf(stderr, "PASS Dis application completed\n");
    }
    return result;
}
