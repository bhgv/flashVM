/* Copyright (c) 2026 bhgv
 * EN: CLI parsing and explicit execution profile validation; included once by main.c.
 * RU: Разбор CLI и проверка явного профиля исполнения; однократно подключается из main.c.
 */


/* EN: Parse borrowed argc/argv on the launch thread and validate immutable
 * execution/GC settings before creating workers. Return the VM status or a
 * nonzero diagnostic on invalid options/startup; never mutate a running profile.
 * RU: Разобрать заимствованные argc/argv в стартовом потоке и проверить настройки
 * исполнения/GC до создания потоков. Вернуть код VM либо ненулевую диагностику
 * ошибки параметров/старта; профиль работающей VM не изменяется. */
int main(int argc, char **argv)
{
    struct sigaction pipe_action = {0};
    int gc_selected = 0;
#ifdef DVM_RUNTIME_EDITOR
    int threaded = 1;
    int threading_selected = 0;
#endif
    /* This executable handles broken pipes as I/O errors on every worker,
     * including a reader disappearing between POLLOUT and splice. */
    pipe_action.sa_handler = SIG_IGN;
    sigemptyset(&pipe_action.sa_mask);
    if (sigaction(SIGPIPE, &pipe_action, NULL))
    {
        perror("disvm: cannot configure broken-pipe handling");
        return 1;
    }
    /* Native pipe writes bypass stdio buffering, so keep host diagnostics
     * in stream order with guest output. */
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    cflag = 1;
    while (argc > 1 && argv[1][0] == '-')
    {
#ifdef DVM_RUNTIME_EDITOR
        if (!strcmp(argv[1], "--threaded-ui") || !strcmp(argv[1], "--sequential-ui"))
        {
            if (threading_selected)
            {
                fprintf(stderr, "Select UI execution mode only once\n");
                return 2;
            }
            threaded = !strcmp(argv[1], "--threaded-ui");
            threading_selected = 1;
            argv++;
            argc--;
            continue;
        }
        if (argc > 2 && dvm_editor_option(argv[1], argv[2]))
        {
            argv += 2;
            argc -= 2;
            continue;
        }
#endif
        if (strcmp(argv[1], "--stats") == 0)
        {
            show_stats = 1;
        }
        else if (strcmp(argv[1], "--max-instructions") == 0)
        {
            size_t limit;
            if (dvm_runtime_instruction_limit || argc < 3 || !runtime_extent(argv[2], &limit) || !limit)
            {
                fprintf(stderr, "--max-instructions requires one decimal value in 1..2147483647\n");
                return 2;
            }
            dvm_runtime_instruction_limit = (unsigned long)limit;
            argv += 2;
            argc -= 2;
            continue;
        }
        else if (strcmp(argv[1], "--quantum") == 0)
        {
            size_t quantum;
            if (quantum_selected || argc < 3 || !runtime_extent(argv[2], &quantum) || !quantum || quantum > 65536)
            {
                fprintf(stderr, "--quantum requires one value in 1..65536 and cannot be repeated\n");
                return 2;
            }
            scheduler_quantum = (int)quantum;
            quantum_selected = 1;
            argv += 2;
            argc -= 2;
            continue;
        }
        else if (strcmp(argv[1], "--inspect-words") == 0 || strcmp(argv[1], "--inspect-reals") == 0)
        {
            if (inspect_requested || argc < 4 || !runtime_extent(argv[2], &inspect_offset) ||
                !runtime_extent(argv[3], &inspect_count) || inspect_count < 1 || inspect_count > 4096 ||
                inspect_offset % 8U != 0)
            {
                fprintf(
                    stderr,
                    "%s requires one aligned decimal byte offset and 1..4096 cells; inspection options cannot be combined\n",
                    argv[1]);
                return 2;
            }
            inspect_reals = strcmp(argv[1], "--inspect-reals") == 0;
            inspect_requested = 1;
            argv += 3;
            argc -= 3;
            continue;
        }
        else if (strcmp(argv[1], "--test-control") == 0)
        {
            test_control = 1;
        }
        else if (strcmp(argv[1], "--gc-locked-poll") == 0)
        {
            gc_locked_poll = 1;
        }
        else if (strcmp(argv[1], "--managed-owner-probe") == 0)
        {
            dvm_managed_owner_probe_requested = 1;
        }
        else if (strcmp(argv[1], "--native-queue-probe") == 0)
        {
            if (!dvm_queue_probe_enable())
            {
                fprintf(stderr, "cannot initialize native queue probe\n");
                return 2;
            }
        }
        else if (strcmp(argv[1], "--native-spawn") == 0)
        {
            native_spawn = 1;
        }
        else if (strcmp(argv[1], "--managed-workers") == 0)
        {
            size_t count = 0;
            if (managed_workers_selected || argc < 3 ||
                (strcmp(argv[2], "auto") && (!runtime_extent(argv[2], &count) || count < 1)))
            {
                fprintf(stderr, "--managed-workers requires 1 for the single-worker reference\n");
                return 2;
            }
            if (count != 1)
            {
                fprintf(stderr, "shared executor pool has been removed; use --native-spawn\n");
                return 2;
            }
            managed_workers_selected = 1;
            argv++;
            argc--;
        }
        else if (strcmp(argv[1], "--managed-workers-limit") == 0)
        {
            fprintf(stderr, "shared executor pool has been removed; --managed-workers-limit is unavailable\n");
            return 2;
        }
        else if (strcmp(argv[1], "--guarded-copies") == 0)
        {
            if (dvm_runtime_guarded_copies)
            {
                fprintf(stderr, "--guarded-copies may be selected only once\n");
                return 2;
            }
            dvm_runtime_guarded_copies = 1;
        }
        else if (strcmp(argv[1], "--parallel-add") == 0 || strcmp(argv[1], "--parallel-integer") == 0)
        {
            if (dvm_runtime_parallel_add)
            {
                fprintf(stderr, "parallel arithmetic mode may be selected only once\n");
                return 2;
            }
            dvm_runtime_parallel_add = strcmp(argv[1], "--parallel-add") == 0 ? 1 : 2;
        }
        else if (strcmp(argv[1], "--parallel-real") == 0)
        {
            dvm_runtime_parallel_real = 1;
        }
        else if (strcmp(argv[1], "--parallel-movp") == 0)
        {
            dvm_runtime_parallel_movp = 1;
        }
        else if (strcmp(argv[1], "--parallel-byte-copies") == 0)
        {
            dvm_runtime_parallel_byte_copies = 1;
        }
        else if (strcmp(argv[1], "--parallel-channel-buffer") == 0)
        {
            dvm_runtime_parallel_channel_buffer = 1;
        }
        else if (strcmp(argv[1], "--parallel-movmp") == 0)
        {
            dvm_runtime_parallel_movmp = 1;
        }
        else if (strcmp(argv[1], "--parallel-slicela") == 0)
        {
            dvm_runtime_parallel_slicela = 1;
        }
        else if (strcmp(argv[1], "--parallel-string") == 0)
        {
            dvm_runtime_parallel_string = 1;
        }
        else if (strcmp(argv[1], "--gc-idle-cycles") == 0)
        {
            dvm_runtime_idle_cycles = 1;
        }
        else if (strcmp(argv[1], "--gc-no-cycles") == 0)
        {
            dvm_runtime_disable_cycles = 1;
        }
        else if (strcmp(argv[1], "--gc-event-cycles") == 0)
        {
            dvm_runtime_event_cycles = 1;
        }
        else if (strcmp(argv[1], "--gc-worker") == 0)
        {
            gc_worker = 1;
            gc_selected = 1;
        }
        else if (strcmp(argv[1], "--gc-inline") == 0)
        {
            gc_worker = 0;
            gc_selected = 1;
        }
        else if (strcmp(argv[1], "--no-jit") == 0)
        {
            cflag = 0;
        }
        else if (strcmp(argv[1], "--jit") == 0)
        {
            cflag = 1;
        }
        else if (strcmp(argv[1], "--version") == 0)
        {
            puts("DisVM runtime " DVM_RUNTIME_VERSION ": experimental NC64, magic 819264, Linux " DVM_RUNTIME_BACKEND);
            return 0;
        }
        else if (strcmp(argv[1], "--help") == 0)
        {
            puts(
                "usage: disvm [--jit|--no-jit] [--stats] [--quantum COUNT] [--max-instructions COUNT] [--test-control] [--inspect-words OFFSET COUNT | --inspect-reals OFFSET COUNT] native-cell.dis");
#ifdef DVM_RUNTIME_EDITOR
            puts(
                "editor options: [--font TTF] [--line-numbers 0|1] (bundled Noto Sans Mono, default 11) [--document UTF8 | --new PATH] [--events FRAMES | --window-events FRAMES | --clipboard-events FRAMES] [--capture PPM]");
#endif
            puts(
                "GC execution: --gc-worker (native collectors), --gc-inline (single-worker reference), --gc-locked-poll (mutex reference polling)");
#if !defined(DVM_RUNTIME_EDITOR)
            puts("Default console profile: one native thread per spawn, with native GC.");
#endif
            puts("Explicit GC and guarded/parallel switches select a custom profile.");
            puts(
                "--native-spawn binds one OS thread per guest and forbids all collector exclusion; unsupported legacy work exits 70.");
            puts(
                "--gc-idle-cycles selects idle tracing in the reference; native-spawn always uses concurrent candidate collection.");
            puts(
                "--gc-no-cycles keeps Fast GC active and disables cycle collection. Unreachable cycles can accumulate.");
            puts("--gc-event-cycles coalesces cycle scans after managed owner loss.");
            puts("Diagnostic: --managed-owner-probe with --gc-worker checks managed roots across native owners");
            puts(
                "Diagnostic: --native-queue-probe with --gc-worker checks native wait withdrawal on an extra OS owner");
            puts(
                "Experimental NC64 cells; legacy .dis modules are rejected. Sys subset; shared operations use object and field synchronization.");
            puts(
                "Managed execution: --native-spawn binds one process per OS thread; --managed-workers 1 selects the single-worker reference.");
#ifdef DVM_RUNTIME_EDITOR
            puts("UI execution: --threaded-ui (default: UI main + Dis OS worker), --sequential-ui (reference)");
#endif
            return 0;
        }
        else
        {
            fprintf(stderr, "unknown runtime option: %s\n", argv[1]);
            return 2;
        }
        argv++;
        argc--;
    }
    if (argc != 2)
    {
        fprintf(stderr, "usage: disvm [--jit|--no-jit] native-cell.dis\n");
        return 2;
    }
#if !defined(DVM_RUNTIME_EDITOR)
    /* The console defaults to dedicated native guests. Explicit diagnostic
     * profiles still select their own runtime settings. */
    if (!native_spawn && !gc_selected && !managed_workers_selected && !dvm_runtime_guarded_copies &&
        !dvm_runtime_parallel_add && !dvm_runtime_parallel_real && !dvm_runtime_parallel_string &&
        !dvm_runtime_parallel_movp && !dvm_runtime_parallel_movmp && !dvm_runtime_parallel_slicela &&
        !dvm_runtime_parallel_channel_buffer && !dvm_runtime_parallel_byte_copies &&
        !dvm_managed_owner_probe_requested && !dvm_queue_probe_requested)
    {
        native_spawn = 1;
    }
#else
    USED(gc_selected);
#endif
    if (native_spawn)
    {
        if (managed_workers_selected || (gc_selected && !gc_worker))
        {
            fprintf(stderr, "--native-spawn requires native GC and selects its own executor lifecycle\n");
            return 2;
        }
        gc_worker = 1;
        if (!dvm_runtime_guarded_copies && !dvm_runtime_parallel_add && !dvm_runtime_parallel_real &&
            !dvm_runtime_parallel_string && !dvm_runtime_parallel_movp && !dvm_runtime_parallel_movmp &&
            !dvm_runtime_parallel_slicela && !dvm_runtime_parallel_channel_buffer && !dvm_runtime_parallel_byte_copies)
        {
            dvm_runtime_guarded_copies = 1;
            dvm_runtime_parallel_add = 2;
            dvm_runtime_parallel_real = 1;
            dvm_runtime_parallel_string = 1;
            dvm_runtime_parallel_movp = 1;
            dvm_runtime_parallel_movmp = 1;
            dvm_runtime_parallel_slicela = 1;
            dvm_runtime_parallel_channel_buffer = 1;
            dvm_runtime_parallel_byte_copies = 1;
        }
        dvm_runtime_shared_integer = dvm_runtime_parallel_add != 0;
        dvm_runtime_guest_unbounded = 1;
        scheduler_quantum = PQUANTA;
    }
    if ((dvm_runtime_parallel_add || dvm_runtime_parallel_real || dvm_runtime_parallel_string) &&
        (!dvm_runtime_guarded_copies || !native_spawn))
    {
        fprintf(stderr, "parallel scalar modes require --guarded-copies and multiple managed workers\n");
        return 2;
    }
    if (dvm_runtime_parallel_movp && (!dvm_runtime_guarded_copies || !native_spawn))
    {
        fprintf(stderr, "--parallel-movp requires --guarded-copies and multiple managed workers\n");
        return 2;
    }
    if ((dvm_runtime_parallel_slicela || dvm_runtime_parallel_movmp || dvm_runtime_parallel_channel_buffer ||
         dvm_runtime_parallel_byte_copies) &&
        (!dvm_runtime_guarded_copies || !native_spawn))
    {
        fprintf(stderr, "parallel mapped copies require --guarded-copies and multiple managed workers\n");
        return 2;
    }
    /* [GC:] EN: Dedicated guests never accept tracing park requests, including
     * explicit --gc-worker. Keep the stopped diagnostic path only for the
     * single-thread reference. Disabled Cycles must not acquire idle policy.
     * RU: Закреплённые гости не паркуются ради трассировки даже с --gc-worker.
     * Остановочный диагностический путь остаётся у однопоточного эталона.
     * Отключённому Cycles не назначать политику idle. */
    if (gc_worker && !dvm_runtime_disable_cycles)
    {
        if (native_spawn || !gc_selected)
        {
            dvm_runtime_idle_cycles = 1;
        }
        if (!gc_selected)
        {
            dvm_runtime_event_cycles = 1;
        }
    }
    if (dvm_runtime_idle_cycles && (!gc_worker || dvm_runtime_disable_cycles))
    {
        fprintf(stderr, "--gc-idle-cycles requires native GC and conflicts with --gc-no-cycles\n");
        return 1;
    }
    if (dvm_runtime_disable_cycles && (!gc_worker || dvm_runtime_event_cycles))
    {
        fprintf(stderr, "--gc-no-cycles requires native GC and conflicts with --gc-event-cycles\n");
        return 1;
    }
    if (dvm_runtime_event_cycles && !gc_worker)
    {
        fprintf(stderr, "--gc-event-cycles requires native GC\n");
        return 2;
    }
    if (dvm_runtime_guarded_copies && !gc_worker)
    {
        fprintf(stderr, "--guarded-copies requires --gc-worker\n");
        return 2;
    }
#ifdef DVM_RUNTIME_EDITOR
    if (native_spawn && !threaded)
    {
        fprintf(stderr, "--native-spawn requires --threaded-ui for the graphical host\n");
        return 2;
    }
#endif
    if (dvm_managed_owner_probe_requested && !gc_worker)
    {
        fprintf(stderr, "--managed-owner-probe requires --gc-worker\n");
        return 2;
    }
    if (dvm_queue_probe_requested && !gc_worker)
    {
        fprintf(stderr, "--native-queue-probe requires --gc-worker\n");
        dvm_queue_probe_finish();
        return 2;
    }
    if (native_spawn && show_stats)
    {
        dvm_runqueue_trace_waits();
    }
    dontcompile = !cflag;
    if (sizeof(WORD) != 8 || sizeof(void *) != 8 || IBY2WD != 8 || offsetof(Alt, ac) != 16 ||
        offsetof(F_Sys_print, s) != 64)
    {
        fprintf(stderr, "native-cell compiler/runtime frame contract mismatch\n");
        return 2;
    }
#ifdef DVM_RUNTIME_EDITOR
    if (threaded)
    {
        return dvm_editor_thread_launch(runtime_execute, argv[1]);
    }
#endif
    return runtime_execute(argv[1]);
}
