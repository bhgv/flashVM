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
    int debug_input = -1; /* Explicit debugger pipe read end. */
    int debug_output = -1; /* Explicit debugger pipe write end. */
    const char *debug_path = NULL; /* Explicit local debugger endpoint, immutable during execution. */
#ifdef DVM_RUNTIME_EDITOR
    int threaded = 1;
    int threading_selected = 0;
#endif
    if (argc > 1 && !strcmp(argv[1], "--gdb"))
    {
        return runtime_launch_gdb(argc, argv);
    }
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
        if (!strcmp(argv[1], "--managed-workers") || !strcmp(argv[1], "--managed-workers-limit") ||
            !strcmp(argv[1], "--gc-inline") || !strcmp(argv[1], "--sequential-ui"))
        {
            fprintf(
                stderr,
                "single-threaded and exclusive execution modes have been removed; native concurrency is mandatory\n");
            return 2;
        }
#ifdef DVM_RUNTIME_EDITOR
        if (!strcmp(argv[1], "--threaded-ui"))
        {
            if (threading_selected)
            {
                fprintf(stderr, "Select UI execution mode only once\n");
                return 2;
            }
            threaded = 1;
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
        else if (strcmp(argv[1], "--debug-fds") == 0)
        {
            size_t input;
            size_t output;
            if (debug_path || debug_input >= 0 || argc < 4 || !runtime_extent(argv[2], &input) ||
                !runtime_extent(argv[3], &output) || input < 3 || output < 3 || input == output)
            {
                fprintf(stderr, "--debug-fds requires separate private input/output FDs >=3\n");
                return 2;
            }
            debug_input = (int)input;
            debug_output = (int)output;
            argv += 3;
            argc -= 3;
            continue;
        }
        else if (strcmp(argv[1], "--debug") == 0)
        {
            if (debug_path || debug_input >= 0 || argc < 3)
            {
                fprintf(stderr, "--debug requires one new local socket pathname\n");
                return 2;
            }
            debug_path = argv[2];
            argv += 2;
            argc -= 2;
            continue;
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
        else if (strcmp(argv[1], "--remote-owner-probe") == 0)
        {
            dvm_remote_managed_probe_requested = 1;
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
                "usage: disvm [--gdb (first)] [--debug PATH|--debug-fds IN OUT] [--jit|--no-jit] [--stats] [--quantum COUNT] [--max-instructions COUNT] [--test-control] [--inspect-words OFFSET COUNT | --inspect-reals OFFSET COUNT] native-cell.dis");
#ifdef DVM_RUNTIME_EDITOR
            puts(
                "editor options: [--font TTF] [--line-numbers 0|1] (bundled Noto Sans Mono, default 11) [--document UTF8 | --new PATH] [--events FRAMES | --window-events FRAMES | --clipboard-events FRAMES] [--capture PPM]");
#endif
            puts(
                "GC execution: concurrent native collectors; --gc-worker is accepted explicitly, --gc-locked-poll uses mutex-protected concurrent polling");
#if !defined(DVM_RUNTIME_EDITOR)
            puts("Default and only execution profile: one native thread per Prog, with concurrent GC.");
#endif
            puts("Diagnostic switches never select single-threaded or exclusive execution.");
            puts("--debug PATH starts a private local debugger socket; --debug-fds INPUT OUTPUT uses separate inherited pipes.");
            puts(
                "--native-spawn binds one OS thread per guest and forbids all collector exclusion; unsupported legacy work exits 70.");
            puts("--gc-idle-cycles selects concurrent candidate collection during idle periods.");
            puts(
                "--gc-no-cycles keeps Fast GC active and disables cycle collection. Unreachable cycles can accumulate.");
            puts("--gc-event-cycles coalesces cycle scans after managed owner loss.");
            puts("Diagnostic: --managed-owner-probe with --gc-worker checks managed roots across native owners");
            puts(
                "Diagnostic: --native-queue-probe with --gc-worker checks native wait withdrawal on an extra OS owner");
            puts(
                "Experimental NC64 cells; legacy .dis modules are rejected. Sys subset; shared operations use object and field synchronization.");
            puts("Native concurrency is mandatory; --native-spawn is a compatibility alias.");
#ifdef DVM_RUNTIME_EDITOR
            puts(
                "UI execution: --threaded-ui (UI main and dedicated Dis OS threads); sequential UI execution is removed");
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
    /* EN: All hosts and diagnostics use dedicated guests. No option can select
     * a reference executor or a stopped collector profile.
     * RU: Все host и диагностика используют закреплённых гостей. Ни один параметр
     * не выбирает эталонный исполнитель или остановленный collector. */
    native_spawn = 1;
    if (native_spawn)
    {
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
        fprintf(stderr, "parallel scalar modes require --guarded-copies and native execution\n");
        return 2;
    }
    if (dvm_runtime_parallel_movp && (!dvm_runtime_guarded_copies || !native_spawn))
    {
        fprintf(stderr, "--parallel-movp requires --guarded-copies and native execution\n");
        return 2;
    }
    if ((dvm_runtime_parallel_slicela || dvm_runtime_parallel_movmp || dvm_runtime_parallel_channel_buffer ||
         dvm_runtime_parallel_byte_copies) &&
        (!dvm_runtime_guarded_copies || !native_spawn))
    {
        fprintf(stderr, "parallel mapped copies require --guarded-copies and native execution\n");
        return 2;
    }
    /* [GC:] EN: Dedicated guests never accept tracing park requests. Explicit
     * cycle policies remain concurrent; disabled Cycles must not acquire idle policy.
     * RU: Закреплённые гости не принимают остановочные запросы трассировки.
     * Явные cycle-политики конкурентны; отключённому Cycles не назначать idle. */
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
    if (debug_input >= 0)
    {
        if (!dvm_debug_start_fds(debug_input, debug_output))
        {
            fprintf(stderr, "cannot start private debugger pipes\n");
            return 2;
        }
        close(debug_input);
        close(debug_output);
    }
    if (debug_path && !dvm_debug_start(debug_path))
    {
        fprintf(stderr, "cannot start private debugger socket: %s\n", debug_path);
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
