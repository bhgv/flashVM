/* SDL2 window adapter for desktop targets. All event/render calls remain on main thread. */
#define SDL_MAIN_HANDLED /* OS entrypoints are supplied in frontend/{linux,windows,macos}. */
#include "window.h"
#include "flashvm/clip_actions.h"
#include "keyboard_sdl.h"
#include <SDL.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <limits.h>
/* Plain copied transition; contains no borrowed VM/SDL pointers. */
typedef struct FVM_DesktopEvent
{
    unsigned int kind;             /* Select a key or primary-pointer transition. */
    FVM_MouseEventType mouse_type; /* Down/move/up when kind is the pointer variant. */
    double x, y;             /* Logical coordinates captured at this event, before stage origin. */
    FVM_KeyboardState state; /* Complete post-transition polling state. */
    int pressed;             /* One for physical down/repeat, zero for release. */
} FVM_DesktopEvent;
/* Own presentation objects in reverse-destruction order; retain no VM pointers. */
struct FVM_DesktopWindow
{
    SDL_Window *window;     /* Native OS window, owned until close. */
    SDL_Renderer *renderer; /* SDL software/accelerated presenter selected by SDL. */
    SDL_Texture *texture;   /* Streaming byte-order-independent RGBA texture. */
    unsigned int logical_width, logical_height; /* Fixed stage extent for SDL normalized touches. */
    int touch_active; /* One selected primary contact; additional fingers do not replace it. */
    SDL_TouchID touch_device;  /* Borrowed numeric identity of the selected touch device. */
    SDL_FingerID touch_finger; /* Borrowed numeric identity of the selected contact. */
    int background; /* Suspend timeline/audio while Android activity is not foreground. */
    FVM_DesktopInput
        input; /* Own host state; SDL logical rendering also maps mouse-event positions. */
    FVM_DesktopEvent
        pending_events[FVM_DESKTOP_PENDING_EVENTS]; /* Native bounded transition FIFO. */
    unsigned long
        event_epoch; /* Focus reset cancels an active entry batch, including reentrant changes. */
    size_t event_head, event_count; /* First pending transition and number of occupied slots. */
    FVM_Result event_error; /* Latched overflow, never silently reported as successful delivery. */
    int pumping_events;     /* Reject recursive host pumping from a script/native handler. */
    int ready; /* Nonzero after a complete upload; permits repaint without another VM tick. */
    unsigned char *last_frame; /* Owned packed RGBA snapshot; never a borrowed VM/raster pointer. */
    size_t last_frame_size;    /* Allocated snapshot bytes, logical width * height * RGBA bytes. */
};

/* Create a fixed logical stage and an independently resizable OS window. */
FVM_DesktopWindow *fvm_desktop_window_open(const char *title, unsigned int width,
                                           unsigned int height)
{
    FVM_DesktopWindow *view;
    SDL_SetMainReady();
    /* Handle real finger events once, independently of SDL's configurable mouse synthesis. */
    SDL_SetHint(FVM_DESKTOP_TOUCH_MOUSE_HINT, "0");
    SDL_SetHint(FVM_DESKTOP_MOUSE_TOUCH_HINT, "0");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
        return 0;
    view = (FVM_DesktopWindow *)calloc(1, sizeof(*view));
    if (!view)
    {
        SDL_Quit();
        return 0;
    }
    view->logical_width = width;
    view->logical_height = height;
    view->input.keyboard.struct_size = sizeof(view->input.keyboard);
    view->window =
        SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (int)width,
                         (int)height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (view->window)
        view->renderer = SDL_CreateRenderer(view->window, -1, 0);
    if (view->renderer)
    {
        if (SDL_RenderSetLogicalSize(view->renderer, (int)width, (int)height) == 0)
            view->texture = SDL_CreateTexture(view->renderer, SDL_PIXELFORMAT_RGBA32,
                                              SDL_TEXTUREACCESS_STREAMING, (int)width, (int)height);
    }
    if (!view->window || !view->renderer || !view->texture)
    {
        /* Keep SDL's diagnostic visible before destroying objects or quitting the subsystem. */
        SDL_Log("FlashVM window initialization: %s", SDL_GetError());
        fvm_desktop_window_close(view);
        return 0;
    }
    if (SDL_SetTextureBlendMode(view->texture, SDL_BLENDMODE_NONE) != 0)
    {
        SDL_Log("FlashVM texture blend mode: %s", SDL_GetError());
        fvm_desktop_window_close(view);
        return 0;
    }
    return view;
}

/* Present the already uploaded image, preserving aspect and clearing letterbox margins. */
static int draw_texture(FVM_DesktopWindow *view)
{
    if (SDL_SetRenderDrawColor(view->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE) != 0)
        return 0;
    if (SDL_RenderClear(view->renderer) != 0)
        return 0;
    if (SDL_RenderCopy(view->renderer, view->texture, 0, 0) != 0)
        return 0;
    SDL_RenderPresent(view->renderer);
    return 1;
}

/* Recreate invalid device storage from our native snapshot, publishing only after
 * creation, blend setup and upload all succeed. No movie/VM execution is entered. */
static int restore_texture(FVM_DesktopWindow *view)
{
    SDL_Texture *replacement =
        SDL_CreateTexture(view->renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
                          (int)view->logical_width, (int)view->logical_height);
    if (!replacement)
        return 0;
    if (SDL_SetTextureBlendMode(replacement, SDL_BLENDMODE_NONE) != 0 ||
        (view->ready &&
         SDL_UpdateTexture(replacement, 0, view->last_frame,
                           (int)(view->logical_width * FVM_DESKTOP_RGBA_BYTES)) != 0))
    {
        SDL_DestroyTexture(replacement);
        return 0;
    }
    SDL_DestroyTexture(view->texture);
    view->texture = replacement;
    return 1;
}

/* An event-triggered draw failure is a host error, never a request to quit. */
static void repaint_texture(FVM_DesktopWindow *view)
{
    if (view->ready && !view->background && !draw_texture(view))
        view->event_error = FVM_ERROR;
}

/* Consume a validated frame using the SDL-owned texture on the main thread. */
static FVM_Result present_sdl(const FVM_RasterSurface *surface, void *userdata)
{
    FVM_DesktopWindow *view = (FVM_DesktopWindow *)userdata;
    size_t row_bytes, frame_bytes;
    unsigned int row;
    unsigned char *snapshot;
    if (surface->width != view->logical_width || surface->height != view->logical_height ||
        surface->stride > INT_MAX)
    {
        SDL_SetError("FlashVM frame does not match the texture extent or pitch");
        return FVM_INVALID_ARGUMENT;
    }
    /* fvm_host_present already bounds dimensions and source extent. Keep a packed
     * native copy so an SDL device reset cannot depend on expired caller memory. */
    row_bytes = (size_t)surface->width * FVM_DESKTOP_RGBA_BYTES;
    frame_bytes = row_bytes * surface->height;
    if (view->last_frame_size != frame_bytes)
    {
        snapshot = (unsigned char *)malloc(frame_bytes);
        if (!snapshot)
        {
            SDL_SetError("FlashVM retained frame allocation failed");
            return FVM_OUT_OF_MEMORY;
        }
        free(view->last_frame);
        view->last_frame = snapshot;
        view->last_frame_size = frame_bytes;
    }
    if (SDL_UpdateTexture(view->texture, 0, surface->pixels, (int)surface->stride) != 0)
        return FVM_ERROR;
    for (row = 0; row < surface->height; ++row)
        memcpy(view->last_frame + row * row_bytes, surface->pixels + row * surface->stride,
               row_bytes);
    view->ready = 1;
    return draw_texture(view) ? FVM_OK : FVM_ERROR;
}

/* Share the bounded frame contract with non-SDL hosts; retain no source storage. */
int fvm_desktop_window_present(FVM_DesktopWindow *view, const FVM_RasterSurface *surface,
                               size_t byte_count)
{
    if (!view)
        return 0;
    if (fvm_host_present(surface, byte_count, present_sdl, view) == FVM_OK)
        return 1;
    if (!SDL_GetError()[0])
        SDL_SetError("FlashVM frame presentation failed");
    return 0;
}

/* Queue one copied event in shared key/pointer order; overflow is latched until close. */
static void queue_input(FVM_DesktopWindow *view, unsigned int kind, FVM_MouseEventType mouse_type,
                        int pressed)
{
    size_t slot;
    FVM_DesktopEvent *queued;
    if (view->event_error != FVM_OK)
        return;
    if (view->event_count == FVM_DESKTOP_PENDING_EVENTS)
    {
        view->event_error = FVM_ERROR;
        return;
    }
    slot = (view->event_head + view->event_count) % FVM_DESKTOP_PENDING_EVENTS;
    queued = view->pending_events + slot;
    queued->kind = kind;
    queued->mouse_type = mouse_type;
    queued->state = view->input.keyboard;
    queued->pressed = pressed;
    queued->x = view->input.x;
    queued->y = view->input.y;
    ++view->event_count;
}

/* SDL's logical-size event filter already maps normalized touches into the logical viewport. */
static void touch_input(FVM_DesktopWindow *view, const SDL_TouchFingerEvent *event)
{
    FVM_MouseEventType type;
    /* Older supported SDK headers do not define mouse-to-finger synthesis identities. */
#ifdef SDL_MOUSE_TOUCHID
    if (event->touchId == SDL_MOUSE_TOUCHID)
        return;
#endif
    if (event->type == SDL_FINGERDOWN)
    {
        if (view->touch_active || (view->input.buttons & FVM_DESKTOP_BUTTON_LEFT))
            return;
        type = FVM_MOUSE_EVENT_DOWN;
    }
    else
    {
        if (!view->touch_active || view->touch_device != event->touchId ||
            view->touch_finger != event->fingerId)
            return;
        type = event->type == SDL_FINGERUP ? FVM_MOUSE_EVENT_UP : FVM_MOUSE_EVENT_MOVE;
    }
    if (!view->logical_width || !view->logical_height ||
        !(event->x >= 0 && event->x <= 1 && event->y >= 0 && event->y <= 1))
    {
        view->event_error = FVM_INVALID_ARGUMENT;
        return;
    }
    view->input.x = (double)event->x * view->logical_width;
    view->input.y = (double)event->y * view->logical_height;
    if (type == FVM_MOUSE_EVENT_DOWN)
    {
        view->touch_device = event->touchId;
        view->touch_finger = event->fingerId;
        view->touch_active = 1;
        view->input.buttons |= FVM_DESKTOP_BUTTON_LEFT;
        ++view->input.clicks;
    }
    else if (type == FVM_MOUSE_EVENT_UP)
    {
        view->touch_active = 0;
        view->input.buttons &= ~FVM_DESKTOP_BUTTON_LEFT;
    }
    queue_input(view, FVM_DESKTOP_EVENT_MOUSE, type, 0);
}

/* Native key/mouse/window events are scoped to our SDL window. ID zero remains
 * accepted for synthetic/unassigned events used by explicit embedding fixtures. */
static int belongs_to_window(FVM_DesktopWindow *view, const SDL_Event *event)
{
    Uint32 id = 0;
    switch (event->type)
    {
    case SDL_WINDOWEVENT:
        id = event->window.windowID;
        break;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        id = event->key.windowID;
        break;
    case SDL_MOUSEMOTION:
        id = event->motion.windowID;
        break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        id = event->button.windowID;
        break;
    default:
        break;
    }
    return !id || id == SDL_GetWindowID(view->window);
}

/* Handle OS events without advancing the VM. Expose/resize can repaint during a slow frame. */
static int handle_event(FVM_DesktopWindow *view, const SDL_Event *event)
{
    if (!belongs_to_window(view, event))
        return 1;
    if (event->type == SDL_QUIT ||
        (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_CLOSE))
        return 0;
    if (event->type == SDL_RENDER_DEVICE_RESET)
    {
        if (!restore_texture(view))
            view->event_error = FVM_ERROR;
        else
            repaint_texture(view);
        return 1;
    }
    if (event->type == SDL_RENDER_TARGETS_RESET)
    {
        /* Streaming textures survive a target reset; only redraw the window. */
        repaint_texture(view);
        return 1;
    }
    if (event->type == SDL_APP_WILLENTERBACKGROUND || event->type == SDL_APP_DIDENTERBACKGROUND)
        view->background = 1;
    if (event->type == SDL_APP_DIDENTERFOREGROUND)
        view->background = 0;
    if (event->type == SDL_APP_WILLENTERBACKGROUND || event->type == SDL_APP_DIDENTERBACKGROUND ||
        (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_FOCUS_LOST))
    {
        view->input.left = view->input.right = view->input.up = view->input.down = 0;
        view->input.space = view->input.key_down = 0;
        view->input.buttons = 0;
        view->touch_active = 0;
        memset(&view->input.keyboard, 0, sizeof(view->input.keyboard));
        view->input.keyboard.struct_size = sizeof(view->input.keyboard);
        view->event_head = view->event_count = 0;
        ++view->event_epoch;
    }
    /* Suppress stale physical transitions until DIDENTERFOREGROUND. Lifecycle,
     * device and close events above remain serviced during suspension. */
    if (view->background)
        return 1;
    if (event->type == SDL_KEYDOWN &&
        (event->key.keysym.sym == SDLK_ESCAPE || event->key.keysym.sym == SDLK_AC_BACK))
        return 0;
    if (event->type == SDL_MOUSEMOTION)
    {
        if (event->motion.which == SDL_TOUCH_MOUSEID || view->touch_active)
            return 1;
        view->input.x = event->motion.x;
        view->input.y = event->motion.y;
        queue_input(view, FVM_DESKTOP_EVENT_MOUSE, FVM_MOUSE_EVENT_MOVE, 0);
    }
    if (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP)
    {
        unsigned int bit;
        if (event->button.which == SDL_TOUCH_MOUSEID || view->touch_active)
            return 1;
        bit = event->button.button == SDL_BUTTON_LEFT     ? FVM_DESKTOP_BUTTON_LEFT
              : event->button.button == SDL_BUTTON_MIDDLE ? FVM_DESKTOP_BUTTON_MIDDLE
              : event->button.button == SDL_BUTTON_RIGHT  ? FVM_DESKTOP_BUTTON_RIGHT
                                                          : 0;
        view->input.x = event->button.x;
        view->input.y = event->button.y;
        if (event->type == SDL_MOUSEBUTTONDOWN)
        {
            view->input.buttons |= bit;
            ++view->input.clicks;
        }
        else
            view->input.buttons &= ~bit;
        if (bit == FVM_DESKTOP_BUTTON_LEFT)
            queue_input(
                view, FVM_DESKTOP_EVENT_MOUSE,
                event->type == SDL_MOUSEBUTTONDOWN ? FVM_MOUSE_EVENT_DOWN : FVM_MOUSE_EVENT_UP, 0);
    }
    if (event->type == SDL_FINGERDOWN || event->type == SDL_FINGERMOTION ||
        event->type == SDL_FINGERUP)
        touch_input(view, &event->tfinger);
    if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP)
    {
        SDL_Keycode key = event->key.keysym.sym;
        int down = event->type == SDL_KEYDOWN;
        int code = key >= SDLK_a && key <= SDLK_z                  ? (int)(key - SDLK_a + 'A')
                   : key >= 0 && key < FVM_DESKTOP_KEY_ASCII_LIMIT ? (int)key
                                                                   : 0;
        fvm_desktop_keyboard_event(&view->input.keyboard, &event->key);
        if (key == SDLK_LEFT)
        {
            view->input.left = down;
            code = FVM_DESKTOP_KEY_LEFT;
        }
        if (key == SDLK_UP)
        {
            view->input.up = down;
            code = FVM_DESKTOP_KEY_UP;
        }
        if (key == SDLK_RIGHT)
        {
            view->input.right = down;
            code = FVM_DESKTOP_KEY_RIGHT;
        }
        if (key == SDLK_DOWN)
        {
            view->input.down = down;
            code = FVM_DESKTOP_KEY_DOWN;
        }
        if (key == SDLK_SPACE)
            view->input.space = down;
        view->input.key_code = code;
        view->input.key_down = down;
        if (fvm_desktop_key_code(key))
            queue_input(view, FVM_DESKTOP_EVENT_KEY, FVM_MOUSE_EVENT_MOVE, down);
        if (down && !event->key.repeat)
            ++view->input.key_presses;
    }
    if (view->ready && event->type == SDL_WINDOWEVENT &&
        (event->window.event == SDL_WINDOWEVENT_EXPOSED ||
         event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED))
        repaint_texture(view);
    return 1;
}

/* Dispatch only the entry batch; callbacks may enqueue later work without an unbounded loop. */
FVM_Result fvm_desktop_window_pump_clip_input(FVM_DesktopWindow *view, FVM_Context *context,
                                              FVM_Object *root, double origin_x, double origin_y)
{
    size_t remaining;
    unsigned long epoch;
    FVM_Result result = FVM_OK;
    FVM_Affine world_to_event = {1, 0, 0, 1, 0, 0};
    if (!view || !context || view->pumping_events ||
        !(origin_x >= -DBL_MAX && origin_x <= DBL_MAX && origin_y >= -DBL_MAX &&
          origin_y <= DBL_MAX))
        return FVM_INVALID_ARGUMENT;
    if (view->event_error != FVM_OK)
        return view->event_error;
    /* SDL already supplied logical pixels; only restore the authored stage origin. */
    world_to_event.x0 = -origin_x;
    world_to_event.y0 = -origin_y;
    view->pumping_events = 1;
    remaining = view->event_count;
    epoch = view->event_epoch;
    while (remaining-- && view->event_count && epoch == view->event_epoch)
    {
        FVM_DesktopEvent event = view->pending_events[view->event_head];
        double world_x, world_y;
        view->event_head = (view->event_head + 1) % FVM_DESKTOP_PENDING_EVENTS;
        --view->event_count;
        result = fvm_host_pointer_to_world(&world_to_event, event.x, event.y, &world_x, &world_y);
        if (result != FVM_OK)
            break;
        if (event.kind == FVM_DESKTOP_EVENT_KEY)
        {
            result = fvm_input_set_pointer(context, world_x, world_y);
            if (result == FVM_OK)
                result =
                    root ? fvm_input_dispatch_key_listeners(context, &event.state, event.pressed)
                         : fvm_input_dispatch_keyboard(context, &event.state, event.pressed);
        }
        else
            result = fvm_input_dispatch_mouse(context, event.mouse_type, world_x, world_y);
        if (result == FVM_OK && root && epoch == view->event_epoch)
        {
            unsigned long clip_event =
                event.kind == FVM_DESKTOP_EVENT_KEY
                    ? (event.pressed ? FVM_CLIP_EVENT_KEY_DOWN : FVM_CLIP_EVENT_KEY_UP)
                : event.mouse_type == FVM_MOUSE_EVENT_MOVE ? FVM_CLIP_EVENT_MOUSE_MOVE
                : event.mouse_type == FVM_MOUSE_EVENT_DOWN ? FVM_CLIP_EVENT_MOUSE_DOWN
                                                           : FVM_CLIP_EVENT_MOUSE_UP;
            result = fvm_movieclip_dispatch_input(context, root, clip_event);
            if (result == FVM_OK && event.kind == FVM_DESKTOP_EVENT_KEY && event.pressed &&
                epoch == view->event_epoch)
            {
                unsigned int key_code = FVM_CLIP_KEY_NONE;
                /* Translate the queued event, not state possibly changed by a previous handler. */
                result = fvm_input_clip_key_code(&event.state, event.pressed, &key_code);
                if (result == FVM_OK && key_code != FVM_CLIP_KEY_NONE)
                    result = fvm_movieclip_dispatch_key_press(context, root, key_code);
            }
        }
        if (result != FVM_OK)
            break;
    }
    view->pumping_events = 0;
    return result == FVM_OK ? view->event_error : result;
}

/* Keep existing hosts on namespace-only delivery unless they explicitly supply a pinned scene root.
 */
FVM_Result fvm_desktop_window_pump_input(FVM_DesktopWindow *view, FVM_Context *context,
                                         double origin_x, double origin_y)
{
    return fvm_desktop_window_pump_clip_input(view, context, 0, origin_x, origin_y);
}

/* Preserve the earlier internal entrypoint for zero-origin hosts, delivering the shared FIFO. */
FVM_Result fvm_desktop_window_pump_keyboard(FVM_DesktopWindow *view, FVM_Context *context)
{
    return fvm_desktop_window_pump_input(view, context, 0, 0);
}

/* SDL_WaitEventTimeout removes an event: inspect it instead of accidentally losing close. */
int fvm_desktop_window_events(FVM_DesktopWindow *view, int timeout_ms)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
        if (!handle_event(view, &event))
            return 0;
    if (timeout_ms > 0 && SDL_WaitEventTimeout(&event, timeout_ms))
        return handle_event(view, &event);
    return 1;
}

/* Read persistent input/presentation failure separately from normal window close. */
FVM_Result fvm_desktop_window_status(FVM_DesktopWindow *view)
{
    return view ? view->event_error : FVM_INVALID_ARGUMENT;
}

/* Copy host state before publishing it into the rooted script-visible HostInput object. */
void fvm_desktop_window_input(FVM_DesktopWindow *view, FVM_DesktopInput *out)
{
    *out = view->input;
}

/* Android lifecycle is a real pause, not a hidden offscreen timeline loop. */
int fvm_desktop_window_active(FVM_DesktopWindow *view)
{
    return !view->background;
}

/* SDL counters are monotonic and keep scheduling independent of adjustable wall-clock dates. */
double fvm_desktop_now(void)
{
    return (double)SDL_GetPerformanceCounter() * FVM_DESKTOP_MILLISECONDS /
           (double)SDL_GetPerformanceFrequency();
}

/* Borrow the current SDL error string for immediate reporting only. */
const char *fvm_desktop_window_error(void)
{
    return SDL_GetError();
}

/* Safe reverse-order cleanup also handles partially initialized windows. */
void fvm_desktop_window_close(FVM_DesktopWindow *view)
{
    if (!view)
        return;
    free(view->last_frame);
    if (view->texture)
        SDL_DestroyTexture(view->texture);
    if (view->renderer)
        SDL_DestroyRenderer(view->renderer);
    if (view->window)
        SDL_DestroyWindow(view->window);
    free(view);
    SDL_Quit();
}
