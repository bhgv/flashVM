/* Copyright (c) 2026 bhgv
 * EN: Shared player playback setup implementation; included in owner-thread dependency order.
 * RU: Shared player playback setup; подключается в порядке owner-thread зависимостей.
 */

    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    FVM_Object *clip = 0;
    FVM_Object *host_input = 0;
    FVM_Value host_value;
    FVM_DesktopInput input;
    FVM_DesktopAudio audio;
    FVM_DesktopPlaybackSession *control = navigation ? navigation->control : 0;
    FVM_DesktopFrameSession *frames = navigation ? navigation->frames : 0;
    FVM_DesktopWindow *window = 0;
    FVM_SWFInfo info;
    FVM_SWFStageBounds bounds;
    FVM_RasterSurface surface;
    FVM_Affine viewport;
    FVM_ExecutionMode mode;
    size_t stream_count = 0;                        /* Validate root stream policy before native setup. */
    double authored, rate, width, height, period, next;
    double tail_deadline;                           /* Bound completion after scheduled final-frame visibility. */
    unsigned long ticks = 0, limit = options->frames;
    unsigned long presentations = 0, refreshes = 0; /* Count successful draws separately from timeline ticks. */
    int update_requested = 0;         /* Consume coalesced requests before presentation, preserving later requests. */
    unsigned int timer_callbacks = 0; /* Receive each active-loop poll's attempted delivery count. */
    unsigned long timer_wait = 0;     /* Cap SDL waiting at the nearest pending timer deadline. */
    int timers_pending = 0;           /* Receive the queue hint without retaining any managed state. */
    int status = 1, pinned = 0, input_pinned = 0;
    memset(&input, 0, sizeof(input));
    memset(&audio, 0, sizeof(audio));
    memset(&surface, 0, sizeof(surface));
    if (!checked(fvm_config_init(&config), "config", path))
    {
        goto cleanup;
    }
    if (!checked(fvm_runtime_create(&config, &runtime), "runtime", path))
    {
        goto cleanup;
    }
    if (!checked(fvm_context_create(runtime, &context), "context", path))
    {
        goto cleanup;
    }
    if (navigation && !checked(fvm_avm1_set_url_rooted(context, request_movie, navigation, 0), "movie loader", path))
    {
        goto cleanup;
    }
    if (!checked(fvm_load_memory(context, bytes, length), "SWF load", path))
    {
        goto cleanup;
    }
    info.struct_size = sizeof(info);
    bounds.struct_size = sizeof(bounds);
    if (!checked(fvm_swf_info(context, &info), "SWF metadata", path))
    {
        goto cleanup;
    }
    if (!checked(fvm_swf_stage_bounds(context, &bounds), "stage rectangle", path))
    {
        goto cleanup;
    }
    if (!checked(fvm_swf_stream_sound_count(context, &stream_count), "root stream validation", path))
    {
        goto cleanup;
    }
    if (!options->mute &&
        !checked(fvm_desktop_audio_validate(context),
                 "root audio policy (codecs/CRC, supported streams, no StartSound mix, prepared-source budget)",
                 path))
    {
        goto cleanup;
    }
    authored = (double)info.frame_rate_fixed8 / FVM_DESKTOP_FIXED8_UNIT;
    if (!fvm_desktop_rate(options, authored, &rate))
    {
        fprintf(stderr, "%s: invalid effective FPS (use --fps for zero-rate SWFs)\n", path);
        goto cleanup;
    }
    width = ((double)bounds.x_max - (double)bounds.x_min) / FVM_SWF_TWIPS_PER_PIXEL;
    height = ((double)bounds.y_max - (double)bounds.y_min) / FVM_SWF_TWIPS_PER_PIXEL;
    if (!(width > 0 && height > 0 && width <= FVM_DESKTOP_MAX_PIXELS && height <= FVM_DESKTOP_MAX_PIXELS))
    {
        fprintf(stderr, "%s: stage must fit 1..4096 pixels per dimension\n", path);
        goto cleanup;
    }
    surface.width = (unsigned int)width;
    if (surface.width < width)
    {
        ++surface.width;
    }
    surface.height = (unsigned int)height;
    if (surface.height < height)
    {
        ++surface.height;
    }
    surface.stride = surface.width * FVM_DESKTOP_RGBA_BYTES;
    surface.pixels = (unsigned char *)malloc(surface.stride * surface.height);
    if (!surface.pixels)
    {
        goto cleanup;
    }
    if (!checked(fvm_standard_install(context), "standard objects", path))
    {
        goto cleanup;
    }
    if (!checked(fvm_standard_keyboard_install(context), "Key polling namespace", path))
    {
        goto cleanup;
    }
    /* Mouse cursor methods start at SWF5; listener methods keep their own SWF6 gate. */
    if (info.version >= FVM_MOUSE_CURSOR_MIN_SWF_VERSION &&
        !checked(fvm_standard_mouse_install(context), "Mouse pointer namespace", path))
    {
        goto cleanup;
    }
    if (info.version >= FVM_SELECTION_MIN_SWF_VERSION &&
        !checked(fvm_standard_selection_install(context), "Selection MovieClip namespace", path))
    {
        goto cleanup;
    }
    /* Generic user events share no native Key/Mouse/Selection registration state. */
    if (info.version >= FVM_BROADCASTER_MIN_SWF_VERSION &&
        !checked(fvm_standard_broadcaster_install(context), "AsBroadcaster namespace", path))
    {
        goto cleanup;
    }
    if (!checked(fvm_movieclip_load_swf_ex(context,
                                           bytes,
                                           length,
                                           "_root",
                                           (options->mute || stream_count) ? FVM_SWF_IMPORT_MUTE_ROOT_STREAMS : 0,
                                           &clip),
                 "scene import",
                 path))
    {
        goto cleanup;
    }
    /* [GC:] Pin the published root before any script/frame operation can collect it. */
    if (!checked(fvm_gc_pin(context, clip), "root pin", path))
    {
        goto cleanup;
    }
    pinned = 1;
    if (navigation)
    {
        navigation->root = clip;
        navigation->audio = &audio;
        if (!checked(
                fvm_movieclip_set_source(context, clip, navigation->initial_base, strlen(navigation->initial_base)),
                "source base",
                path))
        {
            goto cleanup;
        }
    }
    if (!checked(fvm_object_create(context, &host_input), "HostInput object", path))
    {
        goto cleanup;
    }
    /* [GC:] Keep the host-owned input object live even if a script replaces its global binding. */
    if (!checked(fvm_gc_pin(context, host_input), "HostInput pin", path))
    {
        goto cleanup;
    }
    input_pinned = 1;
    fvm_value_undefined(&host_value);
    host_value.type = FVM_VALUE_OBJECT;
    host_value.object = host_input;
    if (!checked(fvm_set_global(context, "HostInput", &host_value), "HostInput binding", path))
    {
        goto cleanup;
    }
    if (navigation && !checked(install_load_functions(context, navigation), "loader functions", path))
    {
        goto cleanup;
    }
    mode = options->mode;
    if (mode == FVM_DESKTOP_MODE_AUTO)
    {
        mode = fvm_jit_available() ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER;
    }
    if (mode == FVM_EXECUTION_JIT && !fvm_jit_available())
    {
        fprintf(stderr, "%s: requested JIT backend unavailable\n", path);
        goto cleanup;
    }
    if (!checked(fvm_execution_set_mode(context, mode, 0), "execution mode", path))
    {
        goto cleanup;
    }
    if (!checked(fvm_stage_set_quality(context, options->quality), "quality", path))
    {
        goto cleanup;
    }
    memset(&viewport, 0, sizeof(viewport));
    viewport.xx = viewport.yy = 1;
    viewport.x0 = -(double)bounds.x_min / FVM_SWF_TWIPS_PER_PIXEL;
    viewport.y0 = -(double)bounds.y_min / FVM_SWF_TWIPS_PER_PIXEL;
    window = frames && frames->offscreen ? fvm_desktop_window_open_offscreen(surface.width, surface.height)
                                         : fvm_desktop_window_open(path, surface.width, surface.height);
    if (!window)
    {
        fprintf(stderr, "%s: cannot initialize presentation: %s\n", path, fvm_desktop_window_error());
        goto cleanup;
    }
    if (navigation && navigation->background)
    {
        navigation->background->window = window;
        navigation->background->audio = &audio;
    }
    if (options->mute &&
        !checked(fvm_movieclip_set_sound_callback(context, muted_sound, &audio), "muted event sound", path))
    {
        goto cleanup;
    }
    if (!options->mute &&
        !checked(fvm_desktop_audio_open_policy(context,
                                               clip,
                                               &audio,
                                               control && control->paused,
                                               options->finite_mixed_tails || options->finite_audio_tails,
                                               options->finite_audio_tails),
                 "event audio (use --mute to disable)",
                 path))
    {
        goto cleanup;
    }
    if (!limit && (options->once || options->file_count > 1))
    {
        limit = info.frame_count;
    }
    fprintf(stderr,
            "%s: SWF%u %ux%u source=%.6g fps effective=%.6g fps\n",
            path,
            info.version,
            surface.width,
            surface.height,
            authored,
            rate);
    period = FVM_DESKTOP_MILLISECONDS / rate;
    next = fvm_desktop_now();
    if (!checked(playback_observe(control, FVM_DESKTOP_PLAYBACK_READY, context, clip, path, ticks, presentations),
                 "playback control",
                 path) ||
        (control && !checked(playback_audio(context, clip, &audio, control), "initial controlled PCM", path)))
    {
        goto cleanup;
    }
    if (control && control->stopped)
    {
        *quit = 1;
    }
