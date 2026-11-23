/* Copyright (c) 2026 bhgv
 * EN: Runtime startup, scheduler loop and joined shutdown; included once by main.c.
 * RU: Запуск runtime, цикл планировщика и завершение после join; однократно подключается из main.c.
 */


/* EN/RU: Startup inputs borrowed until the controller and GUI owners join.
 * Параметры запуска заимствованы до join контроллера и владельца GUI. */
#ifdef DVM_TERMINAL_LAUNCH
#include "styxlaunchlistener_image.h"
#endif

typedef struct RuntimeEntry
{
    const char *path; /* Host-selected local bootstrap path. */
    DVM_LaunchHandoff *handoff; /* Optional exclusive native environment. */
    const char *startup_hex; /* Validated lowercase hex from the native launcher. */
    int launch_listener; /* Boot the embedded Styx listener on the initial resident. */
    int startup_fd; /* Optional sealed owner payload, consumed before dispatch. */
} RuntimeEntry;


/* EN/RU: Read one sealed startup file from offset zero and consume its descriptor.
 * Прочитать запечатанный файл запуска с нулевого смещения и закрыть FD. */
static int read_startup_file(int fd, unsigned char *bytes, size_t *size)
{
    struct stat metadata;
    int seals = fcntl(fd, F_GET_SEALS);
    const int required = F_SEAL_WRITE | F_SEAL_GROW | F_SEAL_SHRINK;
    int valid = seals >= 0 && (seals & required) == required && !fstat(fd, &metadata) &&
        S_ISREG(metadata.st_mode) && metadata.st_size >= 0 && metadata.st_size <= DVM_LAUNCH_ARGUMENT_LIMIT;
    if (valid)
    {
        *size = (size_t)metadata.st_size;
        size_t used = 0;
        while (used < *size)
        {
            ssize_t n = pread(fd, bytes + used, *size - used, (off_t)used);
            if (n > 0)
            {
                used += (size_t)n;
            }
            else if (!n || errno != EINTR)
            {
                valid = 0;
                break;
            }
        }
    }
    close(fd);
    return valid;
}


/* EN/RU: Copy validated arguments into a startup context without changing VFS authority.
 * Скопировать проверенные аргументы в контекст запуска без изменения прав VFS. */
static DVM_LaunchContext *local_startup_context(const RuntimeEntry *entry)
{
    DVM_LaunchHandoff input = {0};
    unsigned char *bytes = malloc(DVM_LAUNCH_ARGUMENT_LIMIT);
    size_t size = 0;
    if (!bytes)
    {
        if (entry->startup_fd >= 0)
        {
            close(entry->startup_fd);
        }
        return NULL;
    }
    int valid = 1;
    if (entry->startup_fd >= 0)
    {
        valid = read_startup_file(entry->startup_fd, bytes, &size);
    }
    else
    {
        size = strlen(entry->startup_hex) / 2;
        for (size_t i = 0; i < size; i++)
        {
            unsigned char high = entry->startup_hex[2 * i];
            unsigned char low = entry->startup_hex[2 * i + 1];
            bytes[i] = ((high <= '9' ? high - '0' : high - 'a' + 10) << 4) |
                (low <= '9' ? low - '0' : low - 'a' + 10);
        }
    }
    DVM_LaunchContext *context = NULL;
    if (valid && strlen(entry->path) < sizeof(input.description.module))
    {
        strcpy(input.description.module, entry->path);
        input.description.argument_bytes = size;
        input.arguments = bytes;
        context = dvm_launch_context_create(&input);
    }
    free(bytes);
    return context;
}


/* EN: Initialize the runtime and drive its controller on the bound native owner.
 * argument points to the borrowed RuntimeEntry until join/return. Native guests run
 * on their permanent threads; their channel operations use the guarded paths.
 * Native collectors cannot stop the world. Return nonzero on startup/runtime
 * failure; the outer wrapper joins services before retiring owner TLS.
 * RU: Инициализировать runtime и вести контроллер на привязанном владельце.
 * argument — заимствованный RuntimeEntry до join/возврата. Гости исполняются на постоянных потоках,
 * каналы используют защищённые операции, native GC не останавливает мир.
 * При ошибке вернуть ненулевой код; внешняя обёртка делает join до удаления TLS. */
static int runtime_execute_inner(const RuntimeEntry *argument)
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
    dvm_unibus_initialize();
    dvm_vfsnative_initialize();
    dvm_cap_initialize();
    dvm_launch_context_initialize();
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
    entry = entry_resource((char *)argument->path);
    if (!dvm_namespace_boot_initialize(module_directory))
    {
        error("cannot initialize startup namespace");
    }
#ifdef DVM_RUNTIME_EDITOR
    if (!dvm_editor_start(module_directory))
    {
        dvm_editor_stop();
        error("cannot initialize graphical editor");
    }
#endif
#ifdef DVM_TERMINAL_LAUNCH
    if (argument->launch_listener)
    {
        Dir metadata = {0};
        uchar image[sizeof(launch_listener_image)];
        memcpy(image, launch_listener_image, sizeof(image));
        module = parsemod("$StyxLaunchListener", image, sizeof(image), &metadata);
    }
    else
#endif
    {
        module = load(entry);
    }
    if (!module)
    {
        error(owner_error());
    }
    if (waserror())
    {
        unload(module);
        nexterror();
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
    process = dvm_runtime_launch_module(module, argument->handoff);
    poperror();
    unload(module);
    initial_guest = process;
    if (argument->startup_hex || argument->startup_fd >= 0)
    {
        DVM_LaunchContext *launch = local_startup_context(argument);
        if (!launch)
        {
            error("cannot prepare startup arguments");
        }
        ((DVM_RuntimeProcess *)process)->launch_context = launch;
    }
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
#ifdef DVM_RUNTIME_EDITOR
    dvm_editor_controller_ready();
#endif
    if (gc_worker && !dvm_close_worker_start())
    {
        error("cannot start native close worker");
    }
    fvm_port_sync_lock(channel_queue_mutex);
    cancel_requests_open = gc_worker && !native_waits_stopping;
    fvm_port_sync_unlock(channel_queue_mutex);
    while ((dvm_debug_enabled && __atomic_load_n(&live, __ATOMIC_RELAXED)) || dvm_runqueue_ready() || (native_spawn && dvm_runqueue_dispatch_pending()) ||
           (native_spawn && (dvm_runqueue_inflight() || dvm_runqueue_mixed_alt_pending() || dvm_result_channels_active()))
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
            if (native_spawn && progress == DVM_MANAGED_TURN_IDLE &&
                (dvm_runqueue_ready() || dvm_runqueue_inflight() || dvm_runqueue_dispatch_pending() ||
                 dvm_runqueue_mixed_alt_pending() || dvm_result_channels_active() ||
#ifdef DVM_RUNTIME_EDITOR
                 dvm_editor_attached() ||
#endif
                 (dvm_debug_enabled && __atomic_load_n(&live, __ATOMIC_RELAXED))))
            {
                if (!dvm_managed_workers_park(dedicated_controller_pending))
                {
                    error("dedicated guest wait failed");
                }
                continue;
            }
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
#ifdef DVM_RUNTIME_EDITOR
    /* EN/RU: Disable UI wake callbacks before the scheduler service is destroyed.
     * Запретить UI-пробуждения до удаления сервиса планировщика. */
    dvm_editor_thread_stopping();
#endif
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
    dvm_namespace_boot_shutdown();
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
