/* Copyright (c) 2026 bhgv
 * EN: Shared player run_modes implementation; included in owner-thread dependency order.
 * RU: Shared player run_modes; подключается в порядке owner-thread зависимостей.
 */


/* Default-provider playlist shares the same import/GC/render/cleanup implementation. */
static int run_background_playlist(const FVM_DesktopOptions *options,
                                   const char **error,
                                   int asynchronous,
                                   const FVM_DesktopLoadMonitor *monitor)
{
    FVM_DesktopOptions value;
    FVM_DesktopBackgroundLoader loader;
    FVM_DesktopSource source;
    size_t i;
    int status = FVM_DESKTOP_EXIT_OK, closed = 0;
    if (!fvm_desktop_options_validate(options, error))
    {
        return FVM_DESKTOP_EXIT_ARGUMENT;
    }
    fvm_desktop_background_loader_init(&loader);
#ifdef FVM_DESKTOP_SOCKET_HTTP
    loader.transfer.fetch = fvm_desktop_socket_transfer_fetch;
#elif defined(FVM_DESKTOP_HTTP)
    loader.transfer.fetch = fvm_desktop_http_transfer_fetch;
#else
    return fvm_desktop_run(options, error);
#endif
    value = *options;
    loader.max_loads = value.max_loads;
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
        status = run_background(&value, &source, &loader, error, &closed, asynchronous, monitor, 0, 0);
        if (status || closed)
        {
            break;
        }
    }
    return status;
}


/* Preserve the explicit event-pumping/waiting mode introduced before asynchronous delivery. */
int fvm_desktop_run_background(const FVM_DesktopOptions *options, const char **error)
{
    return run_background_playlist(options, error, 0, 0);
}


/* Keep child/form transfer completion separate from active timeline/timer execution. */
int fvm_desktop_run_source_async(const FVM_DesktopOptions *options,
                                 const FVM_DesktopSource *source,
                                 const FVM_DesktopBackgroundLoader *loader,
                                 const char **error)
{
    return run_background(options, source, loader, error, 0, 1, 0, 0, 0);
}


/* Async playlist adapter retains ordinary whole-root replacement and close semantics. */
int fvm_desktop_run_async(const FVM_DesktopOptions *options, const char **error)
{
    return run_background_playlist(options, error, 1, 0);
}


/* Zero-cost observer initialization, independent of VM/window/provider startup. */
int fvm_desktop_load_monitor_init(FVM_DesktopLoadMonitor *out)
{
    if (!out)
    {
        return 0;
    }
    out->asynchronous = 1;
    out->timeout_ms = 0;
    out->notify = 0;
    out->userdata = 0;
    return 1;
}


/* Common validation is used by both source and playlist adapters before startup. */
static int load_monitor_valid(const FVM_DesktopLoadMonitor *monitor, const char **error)
{
    if (!monitor || monitor->asynchronous > 1 || monitor->timeout_ms > FVM_DESKTOP_TRANSFER_MAX_TIMEOUT_MS)
    {
        if (error)
        {
            *error = "invalid load monitor policy";
        }
        return 0;
    }
    return 1;
}


/* Validate every observer scalar before routing through the existing owner-thread runner. */
int fvm_desktop_run_source_monitored(const FVM_DesktopOptions *options,
                                     const FVM_DesktopSource *source,
                                     const FVM_DesktopBackgroundLoader *loader,
                                     const FVM_DesktopLoadMonitor *monitor,
                                     const char **error)
{
    if (!load_monitor_valid(monitor, error))
    {
        return FVM_DESKTOP_EXIT_ARGUMENT;
    }
    return run_background(options, source, loader, error, 0, (int)monitor->asynchronous, monitor, 0, 0);
}


/* Thin default-provider adapter preserves the same observation and ownership rules. */
int fvm_desktop_run_monitored(const FVM_DesktopOptions *options,
                              const FVM_DesktopLoadMonitor *monitor,
                              const char **error)
{
    if (!load_monitor_valid(monitor, error))
    {
        return FVM_DESKTOP_EXIT_ARGUMENT;
    }
    return run_background_playlist(options, error, (int)monitor->asynchronous, monitor);
}


/* Validate control before any source/provider/VM/window startup. */
int fvm_desktop_run_source_controlled(const FVM_DesktopOptions *options,
                                      const FVM_DesktopSource *source,
                                      const FVM_DesktopLoader *loader,
                                      const FVM_DesktopPlaybackControl *control,
                                      const char **error)
{
    FVM_DesktopPlaybackSession session;
    if (!fvm_desktop_playback_session_init(&session, control))
    {
        if (error)
        {
            *error = "invalid playback control policy";
        }
        return FVM_DESKTOP_EXIT_ARGUMENT;
    }
    return run_source(options, source, loader, error, 0, 0, &session, 0, 0);
}


/* Preserve explicit worker/monitor policy; callbacks and all VM work remain on the owner. */
int fvm_desktop_run_source_background_controlled(const FVM_DesktopOptions *options,
                                                 const FVM_DesktopSource *source,
                                                 const FVM_DesktopBackgroundLoader *loader,
                                                 const FVM_DesktopLoadMonitor *monitor,
                                                 const FVM_DesktopPlaybackControl *control,
                                                 const char **error)
{
    FVM_DesktopPlaybackSession session;
    if (!fvm_desktop_playback_session_init(&session, control))
    {
        if (error)
        {
            *error = "invalid playback control policy";
        }
        return FVM_DESKTOP_EXIT_ARGUMENT;
    }
    if (!load_monitor_valid(monitor, error))
    {
        return FVM_DESKTOP_EXIT_ARGUMENT;
    }
    return run_background(options, source, loader, error, 0, (int)monitor->asynchronous, monitor, &session, 0);
}


/* Validate both additive policies before any source/provider/native/VM resource is acquired. */
static int observed_sessions(const FVM_DesktopPlaybackControl *control,
                             const FVM_DesktopFrameObserver *observer,
                             FVM_DesktopPlaybackSession *playback,
                             FVM_DesktopFrameSession *frames,
                             const char **error)
{
    FVM_DesktopPlaybackControl defaults;
    if (!fvm_desktop_frame_observer_valid(observer))
    {
        if (error)
        {
            *error = "invalid frame observer policy";
        }
        return 0;
    }
    if (!control)
    {
        fvm_desktop_playback_control_init(&defaults);
        control = &defaults;
    }
    if (!fvm_desktop_playback_session_init(playback, control))
    {
        if (error)
        {
            *error = "invalid playback control policy";
        }
        return 0;
    }
    memset(frames, 0, sizeof(*frames));
    frames->policy = *observer;
    return 1;
}


/* Native C host receives borrowed logical stage rasters without changing old runner behavior. */
int fvm_desktop_run_source_observed(const FVM_DesktopOptions *options,
                                    const FVM_DesktopSource *source,
                                    const FVM_DesktopLoader *loader,
                                    const FVM_DesktopPlaybackControl *control,
                                    const FVM_DesktopFrameObserver *observer,
                                    const char **error)
{
    FVM_DesktopPlaybackSession playback;
    FVM_DesktopFrameSession frames;
    if (!observed_sessions(control, observer, &playback, &frames, error))
    {
        return FVM_DESKTOP_EXIT_ARGUMENT;
    }
    return run_source(options, source, loader, error, 0, 0, &playback, &frames, 0);
}


/* Combine explicit background loading/monitoring with the same owner frame/control callbacks. */
int fvm_desktop_run_source_background_observed(const FVM_DesktopOptions *options,
                                               const FVM_DesktopSource *source,
                                               const FVM_DesktopBackgroundLoader *loader,
                                               const FVM_DesktopLoadMonitor *monitor,
                                               const FVM_DesktopPlaybackControl *control,
                                               const FVM_DesktopFrameObserver *observer,
                                               const char **error)
{
    FVM_DesktopPlaybackSession playback;
    FVM_DesktopFrameSession frames;
    if (!observed_sessions(control, observer, &playback, &frames, error) || !load_monitor_valid(monitor, error))
    {
        return FVM_DESKTOP_EXIT_ARGUMENT;
    }
    return run_background(options, source, loader, error, 0, (int)monitor->asynchronous, monitor, &playback, &frames);
}


/* Windowless owner loop keeps the same scheduling, callbacks and joined cleanup contracts. */
int fvm_desktop_run_source_offscreen(const FVM_DesktopOptions *options,
                                     const FVM_DesktopSource *source,
                                     const FVM_DesktopLoader *loader,
                                     const FVM_DesktopPlaybackControl *control,
                                     const FVM_DesktopFrameObserver *observer,
                                     const char **error)
{
    FVM_DesktopPlaybackSession playback;
    FVM_DesktopFrameSession frames;
    if (!observed_sessions(control, observer, &playback, &frames, error))
    {
        return FVM_DESKTOP_EXIT_ARGUMENT;
    }
    frames.offscreen = 1;
    return run_source(options, source, loader, error, 0, 0, &playback, &frames, 0);
}


/* Windowless resource loading still joins byte workers before releasing VM and raster storage. */
int fvm_desktop_run_source_background_offscreen(const FVM_DesktopOptions *options,
                                                const FVM_DesktopSource *source,
                                                const FVM_DesktopBackgroundLoader *loader,
                                                const FVM_DesktopLoadMonitor *monitor,
                                                const FVM_DesktopPlaybackControl *control,
                                                const FVM_DesktopFrameObserver *observer,
                                                const char **error)
{
    FVM_DesktopPlaybackSession playback;
    FVM_DesktopFrameSession frames;
    if (!observed_sessions(control, observer, &playback, &frames, error) || !load_monitor_valid(monitor, error))
    {
        return FVM_DESKTOP_EXIT_ARGUMENT;
    }
    frames.offscreen = 1;
    return run_background(options, source, loader, error, 0, (int)monitor->asynchronous, monitor, &playback, &frames);
}
