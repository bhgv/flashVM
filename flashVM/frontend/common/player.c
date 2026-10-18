/* Host-owned desktop playback: load, root, advance, rasterize, present, release. */
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include "player.h"
#include "options.h"
#include "window.h"
#include "flashvm/movieclip.h"
#include "flashvm/swf.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Read one bounded UTF-8 path with SDL's platform file adapter, including Unicode Windows paths. */
static unsigned char *read_movie(const char *path, size_t *size)
{
    SDL_RWops *file = SDL_RWFromFile(path, "rb");
    Sint64 length;
    unsigned char *bytes;
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

/* Play one file. All VM/SDL objects belong to this thread; no background worker touches them. */
static int play(const FVM_DesktopOptions *options, const char *path, int *quit)
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    FVM_Object *clip = 0;
    FVM_DesktopWindow *window = 0;
    FVM_SWFInfo info;
    FVM_SWFStageBounds bounds;
    FVM_RasterSurface surface;
    FVM_Affine viewport;
    FVM_ExecutionMode mode;
    unsigned char *bytes = 0;
    size_t length = 0;
    double authored, rate, width, height, period, next;
    unsigned long ticks = 0, limit = options->frames;
    int status = 1, pinned = 0;
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
    if (!checked(fvm_movieclip_load_swf(context, bytes, length, "_root", &clip), "scene import",
                 path))
        goto cleanup;
    /* [GC:] Pin the published root before any script/frame operation can collect it. */
    if (!checked(fvm_gc_pin(context, clip), "root pin", path))
        goto cleanup;
    pinned = 1;
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
        now = fvm_desktop_now();
        if (now < next)
        {
            int wait = (int)(next - now);
            if (wait < 1)
                wait = 1;
            if (!fvm_desktop_window_events(window, wait))
            {
                *quit = 1;
                break;
            }
            continue;
        }
        if (!checked(fvm_movieclip_advance(context, clip, 1), "timeline tick", path))
            goto cleanup;
        if (!clear_stage(context, &surface))
            goto cleanup;
        if (!checked(fvm_movieclip_render_view(context, clip, &surface, &viewport), "raster render",
                     path))
            goto cleanup;
        if (!fvm_desktop_window_present(window, &surface))
        {
            fprintf(stderr, "%s: presentation: %s\n", path, fvm_desktop_window_error());
            goto cleanup;
        }
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
        if (wait < 1)
            wait = 1;
        if (!fvm_desktop_window_events(window, wait))
            *quit = 1;
    }
    if (options->stats)
    {
        FVM_ExecutionStats stats;
        stats.struct_size = sizeof(stats);
        if (!checked(fvm_execution_get_stats(context, &stats), "execution statistics", path))
            goto cleanup;
        fprintf(stderr, "frames=%lu native=%lu interpreter=%lu fallbacks=%lu\n", ticks,
                stats.native_entries, stats.interpreter_entries, stats.fallbacks);
    }
    status = 0;
cleanup:
    fvm_desktop_window_close(window);
    free(surface.pixels);
    free(bytes);
    /* [GC:] End the host root before destroying its context; no borrowed pointer escapes. */
    if (pinned)
        fvm_gc_unpin(context, clip);
    if (context)
        fvm_context_destroy(context);
    if (runtime)
        fvm_runtime_destroy(runtime);
    return status;
}

/* Parse before SDL/VM startup so help and invalid arguments work without a display server. */
int fvm_desktop_main(int argc, char **argv)
{
    FVM_DesktopOptions options;
    const char *error = 0;
    const char **files;
    size_t i;
    int parsed, status = 0, quit = 0;
    files = (const char **)malloc((size_t)argc * sizeof(*files));
    if (!files)
        return 1;
    parsed = fvm_desktop_parse(argc, argv, files, &options, &error);
    if (parsed == FVM_DESKTOP_PARSE_HELP)
        fvm_desktop_help(argv[0]);
    else if (parsed == FVM_DESKTOP_PARSE_ERROR)
    {
        fprintf(stderr, "%s\nUse --help for options.\n", error ? error : "invalid arguments");
        status = 2;
    }
    else
        for (i = 0; i < options.file_count && !quit; ++i)
        {
            status = play(&options, options.files[i], &quit);
            if (status)
                break;
        }
    free(files);
    return status;
}
