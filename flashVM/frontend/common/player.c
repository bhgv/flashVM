/* Host-owned desktop playback: load, root, advance, rasterize, present, release. */
#define SDL_MAIN_HANDLED
#include "player.h"
#include "desktop_build_version.h"
#include "audio.h"
#include "flashvm/gc.h"
#include "flashvm/broadcaster.h"
#include "flashvm/input.h"
#include "flashvm/keyboard.h"
#include "flashvm/mouse.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/timers.h"
#include "flashvm/swf.h"
#include "flashvm/swf_stream_sound.h"
#include "options.h"
#include "window.h"
#include <SDL.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Query linked/build metadata without creating runtime state or probing native devices. */
FVM_Result fvm_desktop_build_info(FVM_DesktopBuildInfo *out)
{
    if (!out)
        return FVM_INVALID_ARGUMENT;
    out->version = FVM_DESKTOP_BUILD_VERSION;
    out->abi_version = fvm_abi_version();
    out->pointer_bits = (unsigned int)(sizeof(void *) * CHAR_BIT);
    out->jit_available = fvm_jit_available() != 0;
#ifdef FVM_HAS_SDL_AUDIO
    out->sdl_audio_compiled = 1;
#else
    out->sdl_audio_compiled = 0;
#endif
    return FVM_OK;
}

/* Console adapter for the same metadata available to a C host; no SDL_Init call. */
static void print_build_info(void)
{
    FVM_DesktopBuildInfo info;
    SDL_version linked;
    fvm_desktop_build_info(&info);
    SDL_GetVersion(&linked);
    printf("FlashVM player %s\n", info.version);
    printf("abi=%lu pointer_bits=%u jit_backend=%s sdl_audio_compiled=%s\n", info.abi_version,
           info.pointer_bits, info.jit_available ? "available" : "unavailable",
           info.sdl_audio_compiled ? "yes" : "no");
    printf("sdl_headers=%u.%u.%u sdl_linked=%u.%u.%u\n", (unsigned int)SDL_MAJOR_VERSION,
           (unsigned int)SDL_MINOR_VERSION, (unsigned int)SDL_PATCHLEVEL,
           (unsigned int)linked.major, (unsigned int)linked.minor, (unsigned int)linked.patch);
    puts("Build metadata only; no device or target-runtime acceptance is implied.");
}

/* Read one bounded UTF-8 path with SDL's platform file adapter, including Unicode Windows paths. */
static unsigned char *read_movie(const char *path, size_t *size)
{
    SDL_RWops *file = SDL_RWFromFile(path, "rb");
    Sint64 length;
    unsigned char *bytes;
    unsigned char
        extra; /* Probe measured EOF so a growing file is not loaded as a silent prefix. */
    if (!file)
    {
        fprintf(stderr, "%s: %s\n", path, SDL_GetError());
        return 0;
    }
    length = SDL_RWsize(file);
    if (length <= 0 || length > (Sint64)FVM_SWF_DEFAULT_LIMIT)
    {
        fprintf(stderr, "%s: empty, unseekable or larger than 64 MiB\n", path);
        SDL_RWclose(file);
        return 0;
    }
    bytes = (unsigned char *)malloc((size_t)length);
    if (!bytes)
    {
        fprintf(stderr, "%s: input allocation failed\n", path);
        SDL_RWclose(file);
        return 0;
    }
    if (SDL_RWread(file, bytes, 1, (size_t)length) != (size_t)length)
    {
        fprintf(stderr, "%s: input read: %s\n", path, SDL_GetError());
        free(bytes);
        SDL_RWclose(file);
        return 0;
    }
    SDL_ClearError();
    if (SDL_RWread(file, &extra, 1, 1) != 0 || SDL_GetError()[0])
    {
        fprintf(stderr, "%s: input changed length or failed at EOF: %s\n", path, SDL_GetError());
        free(bytes);
        SDL_RWclose(file);
        return 0;
    }
    if (SDL_RWclose(file) != 0)
    {
        fprintf(stderr, "%s: input close: %s\n", path, SDL_GetError());
        free(bytes);
        return 0;
    }
    *size = (size_t)length;
    return bytes;
}

/* Print the operation and exact VM status; unsupported features must not be silently skipped. */
static int checked(FVM_Result status, const char *operation, const char *path)
{
    if (status == FVM_OK)
        return 1;
    fprintf(stderr, "%s: %s failed (VM status %d)\n", path, operation, (int)status);
    return 0;
}

/* Clear every stage pixel before rendering so old frames cannot leave trails. */
static int clear_stage(FVM_Context *context, FVM_RasterSurface *surface)
{
    FVM_RasterColor background = {FVM_DESKTOP_WHITE, FVM_DESKTOP_WHITE, FVM_DESKTOP_WHITE,
                                  FVM_DESKTOP_OPAQUE};
    FVM_Result result = fvm_stage_get_background(context, &background);
    unsigned int x, y;
    if (result != FVM_OK && result != FVM_NOT_FOUND)
        return 0;
    for (y = 0; y < surface->height; ++y)
    {
        unsigned char *row = surface->pixels + y * surface->stride;
        for (x = 0; x < surface->width; ++x)
        {
            row[x * FVM_DESKTOP_RGBA_BYTES + FVM_DESKTOP_RED] = background.r;
            row[x * FVM_DESKTOP_RGBA_BYTES + FVM_DESKTOP_GREEN] = background.g;
            row[x * FVM_DESKTOP_RGBA_BYTES + FVM_DESKTOP_BLUE] = background.b;
            row[x * FVM_DESKTOP_RGBA_BYTES + FVM_DESKTOP_ALPHA] = background.a;
        }
    }
    return 1;
}

/* Render the current scene without advancing timeline, input, timers or the authored frame count.
 */
static int present_stage(FVM_Context *context, FVM_Object *clip, FVM_DesktopWindow *window,
                         FVM_RasterSurface *surface, const FVM_Affine *viewport, const char *path)
{
    if (!clear_stage(context, surface))
        return 0;
    if (!checked(fvm_movieclip_render_view(context, clip, surface, viewport), "raster render",
                 path))
        return 0;
    if (!fvm_desktop_window_present(window, surface, surface->stride * surface->height))
    {
        fprintf(stderr, "%s: presentation: %s\n", path, fvm_desktop_window_error());
        return 0;
    }
    return 1;
}

/* Copy a scalar without retaining borrowed storage or invoking user-defined input listeners. */
static FVM_Result input_number(FVM_Context *context, FVM_Object *object, const char *name,
                               double number)
{
    FVM_Value value;
    fvm_value_number(&value, number);
    return fvm_object_set(context, object, name, &value);
}

/* Publish a frame snapshot; HostInput is a player extension, not the complete Flash Key API. */
static FVM_Result publish_input(FVM_Context *context, FVM_Object *object,
                                const FVM_DesktopInput *input, double origin_x, double origin_y)
{
    FVM_Result result;
#define FVM_PUBLISH_INPUT(name, value)                                                             \
    do                                                                                             \
    {                                                                                              \
        result = input_number(context, object, name, (double)(value));                             \
        if (result != FVM_OK)                                                                      \
            return result;                                                                         \
    } while (0)
    FVM_PUBLISH_INPUT("x", input->x + origin_x);
    FVM_PUBLISH_INPUT("y", input->y + origin_y);
    FVM_PUBLISH_INPUT("left", input->left);
    FVM_PUBLISH_INPUT("right", input->right);
    FVM_PUBLISH_INPUT("up", input->up);
    FVM_PUBLISH_INPUT("down", input->down);
    FVM_PUBLISH_INPUT("space", input->space);
    FVM_PUBLISH_INPUT("buttons", input->buttons);
    FVM_PUBLISH_INPUT("keyCode", input->key_code);
    FVM_PUBLISH_INPUT("keyDown", input->key_down);
    FVM_PUBLISH_INPUT("clicks", input->clicks);
    FVM_PUBLISH_INPUT("keyPresses", input->key_presses);
#undef FVM_PUBLISH_INPUT
    result = fvm_input_set_keyboard(context, &input->keyboard);
    return result == FVM_OK
               ? fvm_input_set_pointer(context, input->x + origin_x, input->y + origin_y)
               : result;
}

/* Explicit --mute acknowledges validated root sound events without decoding or opening audio. */
static FVM_Result muted_sound(FVM_Context *context, FVM_Object *clip, size_t ordinal,
                              void *userdata)
{
    FVM_DesktopAudio *audio = (FVM_DesktopAudio *)userdata;
    (void)context;
    (void)clip;
    (void)ordinal;
    ++audio->events;
    return FVM_OK;
}

/* Play one file. All VM/SDL objects belong to this thread; no background worker touches them. */
static int play(const FVM_DesktopOptions *options, const char *path, int *quit)
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    FVM_Object *clip = 0;
    FVM_Object *host_input = 0;
    FVM_Value host_value;
    FVM_DesktopInput input;
    FVM_DesktopAudio audio;
    FVM_DesktopWindow *window = 0;
    FVM_SWFInfo info;
    FVM_SWFStageBounds bounds;
    FVM_RasterSurface surface;
    FVM_Affine viewport;
    FVM_ExecutionMode mode;
    unsigned char *bytes = 0;
    size_t length = 0;
    size_t stream_count = 0; /* Detect unsupported root streaming playback before native setup. */
    double authored, rate, width, height, period, next;
    unsigned long ticks = 0, limit = options->frames;
    unsigned long presentations = 0,
                  refreshes = 0; /* Count successful draws separately from timeline ticks. */
    int update_requested =
        0; /* Consume coalesced requests before presentation, preserving later requests. */
    unsigned int timer_callbacks =
        0;                        /* Receive each active-loop poll's attempted delivery count. */
    unsigned long timer_wait = 0; /* Cap SDL waiting at the nearest pending timer deadline. */
    int timers_pending = 0;       /* Receive the queue hint without retaining any managed state. */
    int status = 1, pinned = 0, input_pinned = 0;
    memset(&input, 0, sizeof(input));
    memset(&audio, 0, sizeof(audio));
    memset(&surface, 0, sizeof(surface));
    bytes = read_movie(path, &length);
    if (!bytes)
        goto cleanup;
    if (!checked(fvm_config_init(&config), "config", path))
        goto cleanup;
    if (!checked(fvm_runtime_create(&config, &runtime), "runtime", path))
        goto cleanup;
    if (!checked(fvm_context_create(runtime, &context), "context", path))
        goto cleanup;
    if (!checked(fvm_load_memory(context, bytes, length), "SWF load", path))
        goto cleanup;
    info.struct_size = sizeof(info);
    bounds.struct_size = sizeof(bounds);
    if (!checked(fvm_swf_info(context, &info), "SWF metadata", path))
        goto cleanup;
    if (!checked(fvm_swf_stage_bounds(context, &bounds), "stage rectangle", path))
        goto cleanup;
    if (!checked(fvm_swf_stream_sound_count(context, &stream_count), "root stream validation",
                 path))
        goto cleanup;
    if (stream_count && !options->mute)
    {
        fprintf(stderr,
                "%s: root streaming audio playback is unsupported; use --mute to play silently\n",
                path);
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
    if (!(width > 0 && height > 0 && width <= FVM_DESKTOP_MAX_PIXELS &&
          height <= FVM_DESKTOP_MAX_PIXELS))
    {
        fprintf(stderr, "%s: stage must fit 1..4096 pixels per dimension\n", path);
        goto cleanup;
    }
    surface.width = (unsigned int)width;
    if (surface.width < width)
        ++surface.width;
    surface.height = (unsigned int)height;
    if (surface.height < height)
        ++surface.height;
    surface.stride = surface.width * FVM_DESKTOP_RGBA_BYTES;
    surface.pixels = (unsigned char *)malloc(surface.stride * surface.height);
    if (!surface.pixels)
        goto cleanup;
    if (!checked(fvm_standard_install(context), "standard objects", path))
        goto cleanup;
    if (!checked(fvm_standard_keyboard_install(context), "Key polling namespace", path))
        goto cleanup;
    /* Mouse cursor methods start at SWF5; listener methods keep their own SWF6 gate. */
    if (info.version >= FVM_MOUSE_CURSOR_MIN_SWF_VERSION &&
        !checked(fvm_standard_mouse_install(context), "Mouse pointer namespace", path))
        goto cleanup;
    if (info.version >= FVM_SELECTION_MIN_SWF_VERSION &&
        !checked(fvm_standard_selection_install(context), "Selection MovieClip namespace", path))
        goto cleanup;
    /* Generic user events share no native Key/Mouse/Selection registration state. */
    if (info.version >= FVM_BROADCASTER_MIN_SWF_VERSION &&
        !checked(fvm_standard_broadcaster_install(context), "AsBroadcaster namespace", path))
        goto cleanup;
    if (!checked(fvm_movieclip_load_swf_ex(context, bytes, length, "_root",
                                           options->mute ? FVM_SWF_IMPORT_MUTE_ROOT_STREAMS : 0,
                                           &clip),
                 "scene import", path))
        goto cleanup;
    /* [GC:] Pin the published root before any script/frame operation can collect it. */
    if (!checked(fvm_gc_pin(context, clip), "root pin", path))
        goto cleanup;
    pinned = 1;
    if (!checked(fvm_object_create(context, &host_input), "HostInput object", path))
        goto cleanup;
    /* [GC:] Keep the host-owned input object live even if a script replaces its global binding. */
    if (!checked(fvm_gc_pin(context, host_input), "HostInput pin", path))
        goto cleanup;
    input_pinned = 1;
    fvm_value_undefined(&host_value);
    host_value.type = FVM_VALUE_OBJECT;
    host_value.object = host_input;
    if (!checked(fvm_set_global(context, "HostInput", &host_value), "HostInput binding", path))
        goto cleanup;
    free(bytes);
    bytes = 0;
    mode = options->mode;
    if (mode == FVM_DESKTOP_MODE_AUTO)
        mode = fvm_jit_available() ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER;
    if (mode == FVM_EXECUTION_JIT && !fvm_jit_available())
    {
        fprintf(stderr, "%s: requested JIT backend unavailable\n", path);
        goto cleanup;
    }
    if (!checked(fvm_execution_set_mode(context, mode, 0), "execution mode", path))
        goto cleanup;
    if (!checked(fvm_stage_set_quality(context, options->quality), "quality", path))
        goto cleanup;
    memset(&viewport, 0, sizeof(viewport));
    viewport.xx = viewport.yy = 1;
    viewport.x0 = -(double)bounds.x_min / FVM_SWF_TWIPS_PER_PIXEL;
    viewport.y0 = -(double)bounds.y_min / FVM_SWF_TWIPS_PER_PIXEL;
    window = fvm_desktop_window_open(path, surface.width, surface.height);
    if (!window)
    {
        fprintf(stderr, "%s: cannot open desktop window\n", path);
        goto cleanup;
    }
    if (options->mute && !checked(fvm_movieclip_set_sound_callback(context, muted_sound, &audio),
                                  "muted event sound", path))
        goto cleanup;
    if (!options->mute && !checked(fvm_desktop_audio_open(context, clip, &audio),
                                   "event audio (use --mute to disable)", path))
        goto cleanup;
    if (!limit && (options->once || options->file_count > 1))
        limit = info.frame_count;
    fprintf(stderr, "%s: SWF%u %ux%u source=%.6g fps effective=%.6g fps\n", path, info.version,
            surface.width, surface.height, authored, rate);
    period = FVM_DESKTOP_MILLISECONDS / rate;
    next = fvm_desktop_now();
    while (!limit || ticks < limit)
    {
        double now;
        if (!fvm_desktop_window_events(window, 0))
        {
            *quit = 1;
            break;
        }
        if (!checked(fvm_desktop_window_status(window), "window events", path))
            goto cleanup;
        if (!fvm_desktop_window_active(window))
        {
            if (audio.output &&
                !checked(fvm_desktop_audio_pause(&audio, 1), "background audio pause", path))
                goto cleanup;
            if (!fvm_desktop_window_events(window, FVM_DESKTOP_AUDIO_WAIT))
                *quit = 1;
            if (!checked(fvm_desktop_window_status(window), "background window events", path))
                goto cleanup;
            if (*quit)
                break;
            next = fvm_desktop_now();
            continue;
        }
        if (!checked(fvm_desktop_audio_tick(&audio), "PCM queue", path))
            goto cleanup;
        if (!checked(fvm_desktop_window_pump_clip_input(window, context, clip, -viewport.x0,
                                                        -viewport.y0),
                     "input listeners", path))
            goto cleanup;
        fvm_desktop_window_input(window, &input);
        if (!checked(publish_input(context, host_input, &input,
                                   (double)bounds.x_min / FVM_SWF_TWIPS_PER_PIXEL,
                                   (double)bounds.y_min / FVM_SWF_TWIPS_PER_PIXEL),
                     "host input", path))
            goto cleanup;
        /* Input may register timers; deliver due work even between timeline frames. */
        if (!checked(fvm_timers_poll(context, &timer_callbacks), "scheduled callbacks", path))
            goto cleanup;
        /* [GC:] Input listeners and HostInput publication also allocate without timers.
         * Step once per foreground turn after all callbacks return; roots stay pinned.
         * The collector threshold decides whether this safe point actually collects. */
        if (!checked(fvm_gc_step(context, 0), "event GC step", path))
            goto cleanup;
        if (!checked(fvm_desktop_window_sync_cursor(window, context), "cursor visibility", path))
            goto cleanup;
        now = fvm_desktop_now();
        if (now < next)
        {
            int wait;
            if (!checked(fvm_stage_take_update(context, &update_requested), "redraw request", path))
                goto cleanup;
            if (update_requested)
            {
                if (!present_stage(context, clip, window, &surface, &viewport, path))
                    goto cleanup;
                ++presentations;
                ++refreshes;
                /* A new request made during rendering must not be lost behind a long frame wait. */
                if (!checked(fvm_stage_get_update(context, &update_requested), "redraw state",
                             path))
                    goto cleanup;
                now = fvm_desktop_now();
                if (update_requested || now >= next)
                    continue;
            }
            wait = (int)(next - now);
            if (audio.output && wait > FVM_DESKTOP_AUDIO_WAIT)
                wait = FVM_DESKTOP_AUDIO_WAIT;
            if (wait < 1)
                wait = 1;
            /* Do not sleep past a pending deadline when the authored frame rate is low. */
            if (!checked(fvm_timers_next_delay(context, (unsigned long)wait, &timer_wait,
                                               &timers_pending),
                         "timer wakeup", path))
                goto cleanup;
            wait = (int)timer_wait;
            if (!wait)
                continue; /* A deadline became due after polling; retry without a blocking wait. */
            if (!fvm_desktop_window_events(window, wait))
            {
                *quit = 1;
                break;
            }
            if (!checked(fvm_desktop_window_status(window), "waiting window events", path))
                goto cleanup;
            continue;
        }
        if (!checked(fvm_movieclip_advance(context, clip, 1), "timeline tick", path))
            goto cleanup;
        /* Deliver timers on the VM thread before rendering callback-driven scene changes. */
        if (!checked(fvm_timers_poll(context, &timer_callbacks), "scheduled callbacks", path))
            goto cleanup;
        if (!checked(fvm_desktop_audio_tick(&audio), "frame PCM queue", path))
            goto cleanup;
        if (!checked(fvm_stage_take_update(context, &update_requested), "frame redraw request",
                     path))
            goto cleanup;
        if (!checked(fvm_desktop_window_sync_cursor(window, context), "frame cursor visibility",
                     path))
            goto cleanup;
        if (!present_stage(context, clip, window, &surface, &viewport, path))
            goto cleanup;
        ++presentations;
        ++ticks;
        /* [GC:] Collect only at a safe host boundary; the root and its library remain pinned. */
        if (!checked(fvm_gc_step(context, 0), "GC step", path))
            goto cleanup;
        next += period;
        if (now - next > FVM_DESKTOP_LATE_FRAMES * period)
            next = now;
    }
    /* Keep the last bounded frame visible for its scheduled duration, with responsive close. */
    while (!*quit && fvm_desktop_now() < next)
    {
        int wait = (int)(next - fvm_desktop_now());
        if (!fvm_desktop_window_active(window))
        {
            if (audio.output &&
                !checked(fvm_desktop_audio_pause(&audio, 1), "tail background audio pause", path))
                goto cleanup;
        }
        else if (!checked(fvm_desktop_audio_tick(&audio), "tail PCM queue", path))
            goto cleanup;
        if (audio.output && wait > FVM_DESKTOP_AUDIO_WAIT)
            wait = FVM_DESKTOP_AUDIO_WAIT;
        if (wait < 1)
            wait = 1;
        if (!fvm_desktop_window_events(window, wait))
            *quit = 1;
        if (!checked(fvm_desktop_window_status(window), "final window events", path))
            goto cleanup;
    }
    if (options->stats)
    {
        FVM_ExecutionStats stats;
        stats.struct_size = sizeof(stats);
        if (!checked(fvm_execution_get_stats(context, &stats), "execution statistics", path))
            goto cleanup;
        fprintf(stderr,
                "input_keys=%lu input_clicks=%lu sound_events=%lu sound_stops=%lu audio_device=%d "
                "audio_underruns=%d\n",
                input.key_presses, input.clicks, audio.events, audio.stops, audio.device != 0,
                SDL_AtomicGet(&audio.underruns));
        fprintf(stderr, "presentations=%lu refreshes=%lu\n", presentations, refreshes);
        fprintf(stderr, "frames=%lu native=%lu interpreter=%lu fallbacks=%lu\n", ticks,
                stats.native_entries, stats.interpreter_entries, stats.fallbacks);
    }
    status = 0;
cleanup:
    /* Release both real-output and explicit-mute registrations before audio userdata cleanup. */
    if (context && clip)
        fvm_movieclip_set_sound_callback(context, 0, 0);
    fvm_desktop_audio_close(&audio);
    fvm_desktop_window_close(window);
    free(surface.pixels);
    free(bytes);
    /* [GC:] End the host root before destroying its context; no borrowed pointer escapes. */
    if (input_pinned)
        fvm_gc_unpin(context, host_input);
    if (pinned)
        fvm_gc_unpin(context, clip);
    if (context)
        fvm_context_destroy(context);
    if (runtime)
        fvm_runtime_destroy(runtime);
    return status;
}

/* Shared synchronous runner; the CLI translates arguments into this same contract. */
int fvm_desktop_run(const FVM_DesktopOptions *options, const char **error)
{
    FVM_DesktopOptions value;
    size_t i;
    int status = FVM_DESKTOP_EXIT_OK, quit = 0;
    if (!fvm_desktop_options_validate(options, error))
        return FVM_DESKTOP_EXIT_ARGUMENT;
    value = *options;
    for (i = 0; i < value.file_count && !quit; ++i)
    {
        status = play(&value, value.files[i], &quit);
        if (status)
            break;
    }
    if (status && error)
        *error = "player playback failed";
    return status;
}

/* Parse before SDL/VM startup so help and invalid arguments work without a display server. */
int fvm_desktop_main(int argc, char **argv)
{
    FVM_DesktopOptions options;
    const char *error = 0;
    const char **files;
    int parsed, status = FVM_DESKTOP_EXIT_OK;
    if (!fvm_desktop_arguments_valid(argc, argv))
    {
        fprintf(stderr, "invalid player argument vector\n");
        return FVM_DESKTOP_EXIT_ARGUMENT;
    }
    files = (const char **)malloc((size_t)argc * sizeof(*files));
    if (!files)
        return FVM_DESKTOP_EXIT_PLAYBACK;
    parsed = fvm_desktop_parse(argc, argv, files, &options, &error);
    if (parsed == FVM_DESKTOP_PARSE_HELP)
        fvm_desktop_help(argv[0]);
    else if (parsed == FVM_DESKTOP_PARSE_VERSION)
        print_build_info();
    else if (parsed == FVM_DESKTOP_PARSE_ERROR)
    {
        fprintf(stderr, "%s\nUse --help for options.\n", error ? error : "invalid arguments");
        status = FVM_DESKTOP_EXIT_ARGUMENT;
    }
    else
    {
        status = fvm_desktop_run(&options, &error);
        if (status == FVM_DESKTOP_EXIT_ARGUMENT)
            fprintf(stderr, "%s\n", error);
    }
    free(files);
    return status;
}
