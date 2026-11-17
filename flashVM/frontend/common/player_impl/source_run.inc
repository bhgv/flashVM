/* Copyright (c) 2026 bhgv
 * EN: Shared player source_run implementation; included in owner-thread dependency order.
 * RU: Shared player source_run; подключается в порядке owner-thread зависимостей.
 */


/* EN: Keep transport and successive VM lifetimes disjoint across root replacements.
 * Borrow all run policies/options through return; each iteration owns its staged bytes.
 * Optional closed/error outputs describe normal stop or desktop failure; owner thread only.
 * RU: Сохранять transport policy между root replacements, освобождая bytes каждой итерации.
 * Options/policies borrowed до return; closed/error — результаты, только owner thread. */
static int run_source(const FVM_DesktopOptions *options,
                      const FVM_DesktopSource *source,
                      const FVM_DesktopLoader *loader,
                      const char **error,
                      int *closed,
                      FVM_DesktopBackgroundSession *background,
                      FVM_DesktopPlaybackSession *control,
                      FVM_DesktopFrameSession *frames,
                      const FVM_DesktopManagedLoader *managed)
{
    FVM_DesktopOptions value;
    FVM_DesktopLoader policy;
    FVM_DesktopNavigation navigation;
    FVM_DesktopDownload download;
    char location[FVM_DESKTOP_MAX_LOCATION + 1], resolved[FVM_DESKTOP_MAX_LOCATION + 1];
    const char *reference, *base = 0;
    const void *bytes;
    unsigned char *file_bytes;
    size_t size;
    unsigned int kind;
    char provenance[FVM_DESKTOP_MAX_LOCATION + 1];
    char *executable_base = 0;
    int quit = 0, status = FVM_DESKTOP_EXIT_ARGUMENT;
    FVM_Result fetched;
    memset(&navigation, 0, sizeof(navigation));
    if (error)
    {
        *error = "invalid source or loader policy";
    }
    if (!source || !location_size(source->location) ||
        (source->kind != FVM_DESKTOP_SOURCE_MEMORY && source->kind != FVM_DESKTOP_SOURCE_FILE &&
         source->kind != FVM_DESKTOP_SOURCE_URL) ||
        (source->kind == FVM_DESKTOP_SOURCE_MEMORY &&
         (!source->bytes || !source->size || source->size > FVM_SWF_DEFAULT_LIMIT)) ||
        (source->kind != FVM_DESKTOP_SOURCE_MEMORY && (source->bytes || source->size)))
    {
        return status;
    }
    fvm_desktop_loader_init(&policy);
    if (loader)
    {
        policy = *loader;
    }
    if (!policy.max_loads || policy.max_loads > FVM_DESKTOP_MAX_LOADS)
    {
        return status;
    }
    if (!memory_options(options, source->location, &value, error))
    {
        return status;
    }
    navigation.managed = managed;
    navigation.background = background;
    navigation.control = control;
    navigation.frames = frames;
    navigation.policy = &policy;
    navigation.remaining = policy.max_loads - 1;
    kind = source->kind;
    reference = source->location;
    for (;;)
    {
        file_bytes = 0;
        memset(&download, 0, sizeof(download));
        download.status = FVM_OK;
        bytes = source->bytes;
        size = source->size;
        status = FVM_DESKTOP_EXIT_PLAYBACK;
        if (kind == FVM_DESKTOP_SOURCE_URL || (managed && kind != FVM_DESKTOP_SOURCE_MEMORY))
        {
            memset(resolved, 0, sizeof(resolved));
            if (background)
            {
                background->kind = FVM_DESKTOP_LOAD_ROOT;
            }
            if (managed)
            {
                fetched = managed->fetch(FVM_DESKTOP_FETCH_CODE, reference, base, collect_movie,
                    &download, resolved, sizeof(resolved), managed->userdata);
            }
            else
            {
                fetched = policy.fetch ? policy.fetch(reference, base, collect_movie,
                    &download, resolved, sizeof(resolved), policy.userdata) : FVM_UNSUPPORTED;
            }
            if (background && background->closed)
            {
                free(download.bytes);
                status = FVM_DESKTOP_EXIT_OK;
                quit = 1;
                break;
            }
            if (fetched != FVM_OK || download.status != FVM_OK || !download.size ||
                !memchr(resolved, 0, sizeof(resolved)) || !location_size(resolved))
            {
                fprintf(stderr,
                        "%s: source fetch failed (transport=%d sink=%d)\n",
                        reference,
                        (int)fetched,
                        (int)download.status);
                free(download.bytes);
                break;
            }
            bytes = download.bytes;
            size = download.size;
        }
        else if (managed)
        {
            memcpy(resolved, reference, strlen(reference) + 1);
        }
        else if (!local_location(reference, kind == FVM_DESKTOP_SOURCE_FILE ? base : 0, resolved))
        {
            break;
        }
        if (kind == FVM_DESKTOP_SOURCE_FILE && !managed)
        {
            file_bytes = read_movie(resolved, &size);
            if (!file_bytes)
            {
                break;
            }
            bytes = file_bytes;
        }
        memcpy(location, resolved, strlen(resolved) + 1);
        navigation.initial_base = location;
        if (!managed && kind == FVM_DESKTOP_SOURCE_MEMORY && location[0] != '/' && location[0] != '\\' &&
            !(strlen(location) > 2 && location[1] == ':' && (location[2] == '/' || location[2] == '\\')) &&
            strncmp(location, "http://", strlen("http://")) && strncmp(location, "https://", strlen("https://")))
        {
            executable_base = FVM_DESKTOP_EXECUTABLE_BASE();
            if (!executable_base || !local_location("embedded.swf", executable_base, provenance))
            {
                /* Hosts with restricted OS path discovery may supply an absolute source name.
                 * Never silently substitute process CWD for the executable directory. */
                navigation.initial_base = "unavailable://executable/embedded.swf";
            }
            else
            {
                navigation.initial_base = provenance;
            }
            SDL_free(executable_base);
        }
        /* The pending reference belongs to the previous VM and is no longer needed. */
        free(navigation.pending);
        free(navigation.pending_base);
        navigation.pending = navigation.pending_base = 0;
        status = play_bytes(&value, location, bytes, size, &quit, &navigation);
        free(file_bytes);
        free(download.bytes);
        if (status || quit || !navigation.pending)
        {
            break;
        }
        reference = navigation.pending;
        base = navigation.pending_base;
        kind = strstr(reference, "://") || (base && strstr(base, "://")) ? FVM_DESKTOP_SOURCE_URL
                                                                         : FVM_DESKTOP_SOURCE_FILE;
    }
    free(navigation.pending);
    free(navigation.pending_base);
    if (closed)
    {
        *closed = quit;
    }
    if (error)
    {
        *error = status ? "source fetch, import, navigation limit or playback failed" : 0;
    }
    return status;
}


/* Public source runner leaves playlist orchestration to the caller. */
int fvm_desktop_run_source(const FVM_DesktopOptions *options,
                           const FVM_DesktopSource *source,
                           const FVM_DesktopLoader *loader,
                           const char **error)
{
    return run_source(options, source, loader, error, 0, 0, 0, 0, 0);
}


/* Initialize an explicitly selected worker provider without native startup. */
int fvm_desktop_background_loader_init(FVM_DesktopBackgroundLoader *out)
{
    if (!out)
    {
        return 0;
    }
    fvm_desktop_transfer_options_init(&out->transfer);
    out->max_loads = 1;
    return 1;
}


/* Private session never escapes this synchronous owner-thread call. */
static int run_background(const FVM_DesktopOptions *options,
                          const FVM_DesktopSource *source,
                          const FVM_DesktopBackgroundLoader *loader,
                          const char **error,
                          int *closed,
                          int asynchronous,
                          const FVM_DesktopLoadMonitor *monitor,
                          FVM_DesktopPlaybackSession *control,
                          FVM_DesktopFrameSession *frames)
{
    FVM_DesktopBackgroundSession session;
    FVM_DesktopLoader policy;
    if (!loader || !loader->transfer.fetch || !loader->transfer.max_bytes ||
        loader->transfer.max_bytes > FVM_DESKTOP_TRANSFER_MAX_BYTES)
    {
        if (error)
        {
            *error = "invalid background provider policy";
        }
        return FVM_DESKTOP_EXIT_ARGUMENT;
    }
    memset(&session, 0, sizeof(session));
    session.options = loader->transfer;
    session.asynchronous = asynchronous;
    session.monitor = monitor;
    session.control = control;
    policy.fetch = background_fetch;
    policy.userdata = &session;
    policy.max_loads = loader->max_loads;
    return run_source(options, source, &policy, error, closed, &session, control, frames, 0);
}


/* Native app/IDE entrypoint keeps provider opt-in explicit instead of moving old callbacks. */
int fvm_desktop_run_source_background(const FVM_DesktopOptions *options,
                                      const FVM_DesktopSource *source,
                                      const FVM_DesktopBackgroundLoader *loader,
                                      const char **error)
{
    return run_background(options, source, loader, error, 0, 0, 0, 0, 0);
}
