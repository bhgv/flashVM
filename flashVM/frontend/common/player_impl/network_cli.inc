/* Copyright (c) 2026 bhgv
 * EN: Shared player network_cli implementation; included in owner-thread dependency order.
 * RU: Shared player network_cli; подключается в порядке owner-thread зависимостей.
 */


/* This host initializer stays linkable in SDKs with no HTTP provider. */
int fvm_desktop_network_policy_init(FVM_DesktopHTTPPolicy *out)
{
    if (!out)
    {
        return 0;
    }
    out->max_redirects = 0;
    out->connect_ms = FVM_NET_CONNECT_MS;
    out->total_ms = FVM_NET_TOTAL_MS;
    out->ca_file = 0;
    return 1;
}


/* Reject malformed limits before a callback, worker, window or input source can be opened. */
static int network_policy_valid(const FVM_DesktopHTTPPolicy *policy, const char **error)
{
    if (!policy || policy->max_redirects > FVM_HTTP_REDIRECT_MAX || !policy->connect_ms ||
        policy->connect_ms > policy->total_ms || policy->total_ms > FVM_NET_MAX_MS)
    {
        if (error)
        {
            *error = "invalid HTTP network policy";
        }
        return 0;
    }
    return 1;
}


/* Keep close propagation private so a stopped source also ends the outer playlist. */
static int run_network_source(const FVM_DesktopOptions *options,
                              const FVM_DesktopSource *source,
                              const FVM_DesktopHTTPPolicy *policy,
                              const FVM_DesktopLoadMonitor *monitor,
                              const char **error,
                              int *closed)
{
#if defined(FVM_DESKTOP_SOCKET_HTTP) || defined(FVM_DESKTOP_HTTP)
    FVM_DesktopBackgroundLoader loader;
    fvm_desktop_background_loader_init(&loader);
#ifdef FVM_DESKTOP_SOCKET_HTTP
    loader.transfer.fetch = fvm_desktop_socket_transfer_policy;
#else
    loader.transfer.fetch = fvm_desktop_http_transfer_policy;
#endif
    loader.transfer.userdata = (void *)policy; /* Provider only borrows the const descriptor. */
    loader.max_loads = options ? options->max_loads : 1;
    return run_background(options, source, &loader, error, closed, (int)monitor->asynchronous, monitor, 0, 0);
#else
    FVM_DesktopLoader loader;
    (void)policy;
    (void)monitor;
    fvm_desktop_loader_init(&loader);
    loader.max_loads = options ? options->max_loads : 1;
    return run_source(options, source, &loader, error, closed, 0, 0, 0, 0);
#endif
}


/* IDE entrypoint uses the same resource FIFO and owner-thread monitor as CLI policy playback. */
int fvm_desktop_run_source_network(const FVM_DesktopOptions *options,
                                   const FVM_DesktopSource *source,
                                   const FVM_DesktopHTTPPolicy *policy,
                                   const FVM_DesktopLoadMonitor *monitor,
                                   const char **error)
{
    if (!network_policy_valid(policy, error) || !load_monitor_valid(monitor, error))
    {
        return FVM_DESKTOP_EXIT_ARGUMENT;
    }
    return run_network_source(options, source, policy, monitor, error, 0);
}


/* Logical-resource budgets and optional once semantics are shared across each source run. */
int fvm_desktop_run_network(const FVM_DesktopOptions *options,
                            const FVM_DesktopHTTPPolicy *policy,
                            const FVM_DesktopLoadMonitor *monitor,
                            const char **error)
{
    FVM_DesktopOptions value;
    FVM_DesktopSource source;
    size_t i;
    int status = FVM_DESKTOP_EXIT_OK, closed = 0;
    if (!network_policy_valid(policy, error) || !load_monitor_valid(monitor, error) ||
        !fvm_desktop_options_validate(options, error))
    {
        return FVM_DESKTOP_EXIT_ARGUMENT;
    }
    value = *options;
    if (value.file_count > 1 && !value.frames)
    {
        value.once = 1;
    }
    source.bytes = 0;
    source.size = 0;
    for (i = 0; i < value.file_count; ++i)
    {
        source.location = value.files[i];
        source.kind = strstr(source.location, "://") ? FVM_DESKTOP_SOURCE_URL : FVM_DESKTOP_SOURCE_FILE;
        status = run_network_source(&value, &source, policy, monitor, error, &closed);
        if (status || closed)
        {
            break;
        }
    }
    return status;
}


/* Shared synchronous runner; the CLI translates arguments into this same contract. */
int fvm_desktop_run(const FVM_DesktopOptions *options, const char **error)
{
    FVM_DesktopOptions value;
    FVM_DesktopSource source;
    FVM_DesktopLoader loader;
    size_t i;
    int status = FVM_DESKTOP_EXIT_OK, closed = 0;
    if (!fvm_desktop_options_validate(options, error))
    {
        return FVM_DESKTOP_EXIT_ARGUMENT;
    }
    value = *options;
    fvm_desktop_loader_init(&loader);
    loader.max_loads = value.max_loads;
#ifdef FVM_DESKTOP_HTTP
    loader.fetch = fvm_desktop_http_fetch;
#elif defined(FVM_DESKTOP_SOCKET_HTTP)
    loader.fetch = fvm_desktop_socket_fetch;
#endif
    if (value.file_count > 1 && !value.frames)
    {
        value.once = 1;
    }
    source.bytes = 0;
    source.size = 0;
    for (i = 0; i < value.file_count; ++i)
    {
        source.location = value.files[i];
        source.kind = strstr(source.location, "://") ? FVM_DESKTOP_SOURCE_URL : FVM_DESKTOP_SOURCE_FILE;
        status = run_source(&value, &source, &loader, error, &closed, 0, 0, 0, 0);
        if (status || closed)
        {
            break;
        }
    }
    return status;
}


/* Console-only coalescing state, borrowed by the same owner-thread C monitor used by IDEs. */
typedef struct FVM_DesktopCLIProgress
{
    unsigned long id; /* Last reported source-run request ordinal. */
    size_t received;  /* Last emitted body-byte count; WAIT duplicates are suppressed. */
} FVM_DesktopCLIProgress;


/* Numeric metadata avoids printing credentials or control characters from an input URL. */
static int cli_load_progress(const FVM_DesktopLoadEvent *event, void *userdata)
{
    FVM_DesktopCLIProgress *state = (FVM_DesktopCLIProgress *)userdata;
    if (event->phase != FVM_DESKTOP_LOAD_WAIT || event->id != state->id || event->received != state->received)
    {
        fprintf(stderr,
                "load id=%lu kind=%u phase=%u received=%lu result=%d\n",
                event->id,
                event->kind,
                event->phase,
                (unsigned long)event->received,
                (int)event->result);
    }
    state->id = event->id;
    state->received = event->received;
    return 0;
}


/* Strict decimal integer milliseconds, independent of locale/sign/exponent and unsigned width. */
static int cli_load_timeout(const char *text, unsigned long *out)
{
    unsigned long value = 0;
    size_t length = 0;
    if (!text || !*text)
    {
        return 0;
    }
    while (*text)
    {
        unsigned int digit;
        if (++length > FVM_DESKTOP_DECIMAL_MAX_TEXT || *text < '0' || *text > '9')
        {
            return 0;
        }
        digit = (unsigned int)(*text++ - '0');
        if (value > (FVM_DESKTOP_TRANSFER_MAX_TIMEOUT_MS - digit) / 10UL)
        {
            return 0;
        }
        value = value * 10UL + digit;
    }
    *out = value;
    return 1;
}


/* Parse before SDL/VM startup so help and invalid arguments work without a display server. */
int fvm_desktop_main(int argc, char **argv)
{
    FVM_DesktopOptions options;
    FVM_DesktopLoadMonitor monitor;
    FVM_DesktopHTTPPolicy network;
    unsigned long redirects;
    int redirects_selected = 0, ca_selected = 0;
    FVM_DesktopCLIProgress progress;
    int timeout_selected = 0, progress_selected = 0;
    const char *error = 0;
    const char **files;
    char **arguments;
    int i, count = 1, literal = 0, background = 0;
    int parsed, status = FVM_DESKTOP_EXIT_OK;
    if (!fvm_desktop_arguments_valid(argc, argv))
    {
        fprintf(stderr, "invalid player argument vector\n");
        return FVM_DESKTOP_EXIT_ARGUMENT;
    }
    files = (const char **)malloc((size_t)argc * sizeof(*files));
    if (!files)
    {
        return FVM_DESKTOP_EXIT_PLAYBACK;
    }
    arguments = (char **)malloc((size_t)argc * sizeof(*arguments));
    if (!arguments)
    {
        free(files);
        return FVM_DESKTOP_EXIT_PLAYBACK;
    }
    fvm_desktop_load_monitor_init(&monitor);
    fvm_desktop_network_policy_init(&network);
    memset(&progress, 0, sizeof(progress));
    arguments[0] = argv[0];
    for (i = 1; i < argc; ++i)
    {
        if (!literal && !strcmp(argv[i], "--http-ca-file"))
        {
            if (ca_selected || i + 1 >= argc || !location_size(argv[i + 1]))
            {
                error = "invalid or duplicate --http-ca-file";
                goto argument_failure;
            }
            ca_selected = 1;
            network.ca_file = argv[++i];
        }
        else if (!literal && !strcmp(argv[i], "--redirects"))
        {
            if (redirects_selected || i + 1 >= argc || !cli_load_timeout(argv[i + 1], &redirects) ||
                redirects > FVM_HTTP_REDIRECT_MAX)
            {
                error = "invalid or duplicate --redirects (0..8 integer hops)";
                goto argument_failure;
            }
            redirects_selected = 1;
            network.max_redirects = (unsigned int)redirects;
            ++i;
        }
        else if (!literal && !strcmp(argv[i], "--load-timeout"))
        {
            if (timeout_selected || i + 1 >= argc || !cli_load_timeout(argv[i + 1], &monitor.timeout_ms))
            {
                error = "invalid or duplicate --load-timeout (0..2147483647 integer ms)";
                goto argument_failure;
            }
            timeout_selected = 1;
            ++i;
        }
        else if (!literal && !strcmp(argv[i], "--load-progress"))
        {
            if (progress_selected)
            {
                error = "duplicate --load-progress";
                goto argument_failure;
            }
            progress_selected = 1;
            monitor.notify = cli_load_progress;
            monitor.userdata = &progress;
        }
        else if (!literal && (!strcmp(argv[i], "--background-loads") || !strcmp(argv[i], "--async-loads")))
        {
            if (background)
            {
                fprintf(stderr, "duplicate --background-loads/--async-loads selection\n");
                free(arguments);
                free(files);
                return FVM_DESKTOP_EXIT_ARGUMENT;
            }
            background = !strcmp(argv[i], "--async-loads") ? 2 : 1;
        }
        else
        {
            if (!strcmp(argv[i], "--"))
            {
                literal = 1;
            }
            arguments[count++] = argv[i];
        }
    }
    if ((timeout_selected || progress_selected || redirects_selected || ca_selected) && !background)
    {
        error = "--load-timeout/--load-progress/--redirects/--http-ca-file requires --async-loads or "
                "--background-loads";
        goto argument_failure;
    }
    monitor.asynchronous = background == 2;
    parsed = fvm_desktop_parse(count, arguments, files, &options, &error);
    if (parsed == FVM_DESKTOP_PARSE_HELP)
    {
        fvm_desktop_help(argv[0]);
    }
    else if (parsed == FVM_DESKTOP_PARSE_VERSION)
    {
        print_build_info();
    }
    else if (parsed == FVM_DESKTOP_PARSE_ERROR)
    {
        fprintf(stderr, "%s\nUse --help for options.\n", error ? error : "invalid arguments");
        status = FVM_DESKTOP_EXIT_ARGUMENT;
    }
    else
    {
        status = (redirects_selected || ca_selected) ? fvm_desktop_run_network(&options, &network, &monitor, &error)
                 : (timeout_selected || progress_selected) ? fvm_desktop_run_monitored(&options, &monitor, &error)
                 : background == 2                         ? fvm_desktop_run_async(&options, &error)
                 : background == 1                         ? fvm_desktop_run_background(&options, &error)
                                                           : fvm_desktop_run(&options, &error);
        if (status == FVM_DESKTOP_EXIT_ARGUMENT)
        {
            fprintf(stderr, "%s\n", error);
        }
    }
    free(arguments);
    free(files);
    return status;
argument_failure:
    fprintf(stderr, "%s\n", error);
    free(arguments);
    free(files);
    return FVM_DESKTOP_EXIT_ARGUMENT;
}
