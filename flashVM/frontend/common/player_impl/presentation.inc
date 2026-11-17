/* Copyright (c) 2026 bhgv
 * EN: Shared player presentation implementation; included in owner-thread dependency order.
 * RU: Shared player presentation; подключается в порядке owner-thread зависимостей.
 */


/* One copied native observer policy and run-wide presentation count, shared across roots. */
typedef struct FVM_DesktopFrameSession
{
    int offscreen;                     /* Select timer-only delivery, leaving host video/events/cursor untouched. */
    FVM_DesktopFrameObserver policy;   /* Shallow immutable copy; host userdata remains borrowed. */
    unsigned long total_presentations; /* Successful raster/native presentations for this source run. */
} FVM_DesktopFrameSession;


/* Notify after a completed draw, never while renderer callbacks or native presentation are active.
 */
static int frame_observe(FVM_DesktopFrameSession *frames,
                         const FVM_RasterSurface *surface,
                         const char *source,
                         unsigned long ticks,
                         unsigned long total_ticks,
                         unsigned long presentations,
                         unsigned int reason)
{
    FVM_DesktopFrame frame;
    if (!frames)
    {
        return 1;
    }
    memset(&frame, 0, sizeof(frame));
    frame.struct_size = sizeof(frame);
    frame.format = FVM_DESKTOP_FRAME_RGBA8;
    frame.reason = reason;
    frame.width = surface->width;
    frame.height = surface->height;
    frame.stride = surface->stride;
    frame.byte_count = surface->stride * surface->height;
    frame.pixels = surface->pixels;
    frame.source = source;
    frame.movie_ticks = ticks;
    frame.total_ticks = total_ticks;
    frame.presentations = presentations;
    frame.total_presentations = ++frames->total_presentations;
    return checked(frames->policy.notify(&frame, frames->policy.userdata), "frame observer", source);
}


/* Clear every stage pixel before rendering so old frames cannot leave trails. */
static int clear_stage(FVM_Context *context, FVM_RasterSurface *surface)
{
    FVM_RasterColor background = {FVM_DESKTOP_WHITE, FVM_DESKTOP_WHITE, FVM_DESKTOP_WHITE, FVM_DESKTOP_OPAQUE};
    FVM_Result result = fvm_stage_get_background(context, &background);
    unsigned int x, y;
    if (result != FVM_OK && result != FVM_NOT_FOUND)
    {
        return 0;
    }
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
static int present_stage(FVM_Context *context,
                         FVM_Object *clip,
                         FVM_DesktopWindow *window,
                         FVM_RasterSurface *surface,
                         const FVM_Affine *viewport,
                         const char *path)
{
    if (!clear_stage(context, surface))
    {
        return 0;
    }
    if (!checked(fvm_movieclip_render_view(context, clip, surface, viewport), "raster render", path))
    {
        return 0;
    }
    if (!fvm_desktop_window_present(window, surface, surface->stride * surface->height))
    {
        fprintf(stderr, "%s: presentation: %s\n", path, fvm_desktop_window_error());
        return 0;
    }
    return 1;
}


/* Copy a scalar without retaining borrowed storage or invoking user-defined input listeners. */
static FVM_Result input_number(FVM_Context *context, FVM_Object *object, const char *name, double number)
{
    FVM_Value value;
    fvm_value_number(&value, number);
    return fvm_object_set(context, object, name, &value);
}


/* Publish a frame snapshot; HostInput is a player extension, not the complete Flash Key API. */
static FVM_Result
publish_input(FVM_Context *context, FVM_Object *object, const FVM_DesktopInput *input, double origin_x, double origin_y)
{
    FVM_Result result;
#define FVM_PUBLISH_INPUT(name, value)                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        result = input_number(context, object, name, (double)(value));                                                 \
        if (result != FVM_OK)                                                                                          \
            return result;                                                                                             \
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
    return result == FVM_OK ? fvm_input_set_pointer(context, input->x + origin_x, input->y + origin_y) : result;
}


/* Explicit --mute acknowledges validated root sound events without decoding or opening audio. */
static FVM_Result muted_sound(FVM_Context *context, FVM_Object *clip, size_t ordinal, void *userdata)
{
    FVM_DesktopAudio *audio = (FVM_DesktopAudio *)userdata;
    (void)context;
    (void)clip;
    (void)ordinal;
    ++audio->events;
    return FVM_OK;
}
