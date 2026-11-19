/* Copyright (c) 2026 bhgv
 * EN: Frame presentation, input barriers and paint scheduling; included by the owner translation unit.
 * RU: Показ кадров, входные барьеры и планирование рисования; включается в translation unit владельца.
 */
/* Collect the newest detached raster result; owner-only, wait is explicit capture mode.
 * Return one for ready or idle, zero on native failure. Never publish a stale-size frame. */
static int collect_frame(int wait)
{
    void *job = NULL;
    FVM_Result rendered;
    FVM_Result result = fvm_render_queue_take(raster_queue, &job, &rendered, wait);
    const FVM_RasterSurface *surface;
    int ok = 1;
    if (result == FVM_NOT_FOUND)
    {
        return 1;
    }
    if (result != FVM_OK)
    {
        return 0;
    }
    surface = dvm_editor_frame_surface(job);
    raster_pending = 0;
    if (rendered != FVM_OK)
    {
        fprintf(stderr, "editor background raster failed: %d\n", rendered);
        ok = 0;
    }
    else if (surface->width == canvas_width && surface->height == canvas_height)
    {
        FVM_RasterSurface destination = {pixels, canvas_width, canvas_height, (size_t)canvas_width * 4U};
        const FVM_RasterSurface *previous = dvm_editor_frame_surface(displayed_frame);
        size_t copied;
        int fresh = !pixels_valid || !previous || previous->width != canvas_width ||
                    previous->height != canvas_height || !dvm_editor_frame_has_baseline(job, displayed_frame);
        if (!dvm_editor_frame_present(job, &destination, fresh, &copied))
        {
            dvm_editor_frame_destroy(job);
            return 0;
        }
        if ((events || getenv("DVM_EDITOR_RENDER_STATS")) && dvm_desktop_active())
        {
            unsigned int built, reused, dirty;
            dvm_editor_frame_scene_stats(job, &built, &reused, &dirty);
            unsigned int tiles = 0;
            unsigned int root_pixels = 0;
            unsigned int index;
            for (index = 0; index < DVM_EDITOR_FRAME_MAX_TILES; ++index)
            {
                unsigned int x, y, tile_width, tile_height;
                if (dvm_editor_frame_damage(job, index, &x, &y, &tile_width, &tile_height))
                {
                    tiles++;
                    root_pixels += tile_width * tile_height;
                }
            }
            printf("SCENECACHE %lu %u %u %u %lu\n",
                   dvm_desktop_ticket(),
                   built,
                   reused,
                   dirty,
                   (unsigned long)dvm_editor_frame_scene_pixels(job));
            printf("ROOTDAMAGE %lu %u %u\n", dvm_desktop_ticket(), tiles, root_pixels);
            printf("FRAMECOPY %lu %lu %lu\n",
                   dvm_desktop_ticket(),
                   (unsigned long)dvm_editor_frame_baseline_bytes(job),
                   (unsigned long)copied);
        }
        pixels_valid = 0;
        upload_frame = fresh ? NULL : job;
        ok = present_pixels();
        upload_frame = NULL;
        if (ok)
        {
            pixels_valid = 1;
            dvm_editor_frame_destroy(displayed_frame);
            displayed_frame = job;
            job = NULL;
        }
    }
    dvm_editor_frame_destroy(job);
    return ok;
}

/* Prepare independent native geometry and submit without waiting for rasterization.
 * [GC:] All VM/font queries and collection remain on this owner. Worker paths have
 * no heap references. Capture deliberately waits; replay input advances only after owner presentation. */
static int render(void)
{
    DVM_EditorFrame *frame = dvm_editor_frame_create(canvas_width, canvas_height);
    int prepared;
    if (!frame)
    {
        return 0;
    }
    recording_frame = frame;
    prepared = prepare_frame();
    recording_frame = NULL;
    /* [GC:] Layout capture owns detached data. Honor the allocation threshold,
     * as the desktop path does, instead of sweeping on every typing repaint. */
    if (!prepared || fvm_gc_step(context, 0) != FVM_OK || fvm_render_queue_submit(raster_queue, frame) != FVM_OK)
    {
        dvm_editor_frame_destroy(frame);
        return 0;
    }
    raster_pending = 1;
    return collect_frame(capture_path != NULL);
}

/* Paint the latest complete desktop on its sole VM owner and enqueue copied pixels.
 * Returns one on success, zero on allocation/render failure. No managed object enters
 * a worker; superseded snapshots are replaced before this expensive operation. */
static int render_desktop(void)
{
    const char *retained = getenv("DVM_EDITOR_RETAINED");
    int vector = retained && !strcmp(retained, "0");
    int prepared;
    DVM_EditorFrame *frame = dvm_editor_frame_create(canvas_width, canvas_height);
    if (!frame)
    {
        return 0;
    }
    prepared = vector ? dvm_desktop_prepare_vector(frame) : dvm_desktop_prepare_cached(frame, displayed_frame);
    prepared = prepared && dvm_editor_frame_incremental(frame, displayed_frame);
    if (!prepared && !vector && dvm_editor_frame_scene_budget(frame))
    {
        dvm_editor_frame_destroy(frame);
        frame = dvm_editor_frame_create(canvas_width, canvas_height);
        prepared = frame && dvm_desktop_prepare_vector(frame) && dvm_editor_frame_incremental(frame, displayed_frame);
    }
    /* [GC:] Captured data is detached; live MovieClips remain pinned by their
     * owner tree. Honor the allocation threshold instead of sweeping each drag. */
    if (!prepared || fvm_gc_step(context, 0) != FVM_OK || fvm_render_queue_submit(raster_queue, frame) != FVM_OK)
    {
        dvm_editor_frame_destroy(frame);
        return 0;
    }
    desktop_dirty = 0;
    raster_pending = 1;
    return collect_frame(capture_path != NULL);
}

#ifdef DVM_EDITOR_SDL


/* Inspect only the FIFO head without removing or skipping an event. Keyboard/text
 * input uses completed Dis state rather than pixel hit geometry. Pointer, focus,
 * resize and quit retain presentation ordering. Must run on the SDL/VM owner. */
static int keyboard_waiting(void)
{
    SDL_Event event;
    SDL_PumpEvents();
    if (SDL_PeepEvents(&event, 1, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) != 1)
    {
        return 0;
    }
    return event.type == SDL_TEXTINPUT || event.type == SDL_KEYDOWN || event.type == SDL_KEYUP;
}

/* Inspect only the queued head during an established drawing capture. New hits
 * still require presented geometry; no event is consumed or crosses a release. */
static int paint_motion_waiting(void)
{
    SDL_Event event;
    return !postponed_valid && vector_drag &&
           SDL_PeepEvents(&event, 1, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) == 1 &&
           (event.type == SDL_MOUSEMOTION || event.type == SDL_FINGERMOTION);
}

/* Return whether native input must wait for matching pixels. Keyboard/text can
 * use the latest acknowledged Dis view. Replay/capture stays deterministic, and
 * FIFO inspection never lets a key overtake pointer, focus or lifecycle events. */
static int presentation_blocks_input(void)
{
    int continuation = 0;
    SDL_Event event;
    int keyboard = keyboard_waiting();
    if (postponed_valid)
    {
        return desktop_dirty || raster_pending;
    }
    if ((header_drag || vector_drag || desktop_pan) &&
        SDL_PeepEvents(&event, 1, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) == 1)
    {
        continuation =
            (event.type == SDL_MOUSEMOTION && (event.motion.state & (SDL_BUTTON_LMASK | SDL_BUTTON_MMASK))) ||
            (event.type == SDL_MOUSEBUTTONUP &&
             (event.button.button == SDL_BUTTON_LEFT || event.button.button == SDL_BUTTON_MIDDLE)) ||
            (vector_drag && (event.type == SDL_FINGERMOTION || event.type == SDL_FINGERUP));
    }
    return dvm_editor_pointer_waits(
        desktop_dirty, raster_pending, events != NULL, capture_path != NULL, keyboard, continuation);
}
#endif


/* Frame-free notification only. Arguments are borrowed and ignored; no managed
 * data is retained or rendered inside this native callback. Owner-only. */
static FVM_Result
paint_ready(FVM_Context *c, FVM_Object *receiver, unsigned count, const FVM_Value *args, FVM_Value *out, void *user)
{
    (void)c;
    (void)receiver;
    (void)count;
    (void)args;
    (void)user;
    paint_due = 1;
    fvm_value_undefined(out);
    return FVM_OK;
}

/* Coalesce notifications for the latest complete Dis view. Return0 on failure.
 * [GC:] Pin a native notification until teardown; the queue separately roots it.
 * This only posts work. No raster/layout/Dis mutation happens synchronously. */
static int request_paint(void)
{
    unsigned long id;
    if (!desktop_dirty)
    {
        if (!paint_callback &&
            (fvm_function_create_native(context, paint_ready, NULL, NULL, &paint_callback) != FVM_OK ||
             fvm_gc_pin(context, paint_callback) != FVM_OK))
        {
            return 0;
        }
        if (fvm_async_post(context, paint_callback, 0, NULL, &id) != FVM_OK)
        {
            return 0;
        }
#ifdef DVM_EDITOR_SDL
        desktop_dirty_since = SDL_GetTicks();
#endif
    }
    desktop_dirty = 1;
    return 1;
}
