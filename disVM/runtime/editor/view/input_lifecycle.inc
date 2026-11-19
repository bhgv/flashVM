/* Copyright (c) 2026 bhgv
 * EN: Pointer routing, event composition and resource teardown; included by the owner translation unit.
 * RU: Маршрутизация указателя, сборка событий и удаление ресурсов; включается в translation unit владельца.
 */
/* Resolve a copied raw pointer event using only the completed actual glyph layout.
 * message is replaced with a value request carrying revision and view ticket; Dis decides policy. */
static void resolve_pointer(DVM_UIMessage *message)
{
    if (message->kind != DVM_UI_POINTER)
    {
        return;
    }
    if (dvm_desktop_active())
    {
        char metadata[33];
        unsigned long widget = 0;
        long position = -1;
        unsigned long phase = message->flags;
        unsigned long code = message->code;
        message->kind = DVM_UI_REQUEST;
        message->id = 1;
        message->flags = dvm_desktop_ticket();
        if (code == EDITOR_RESIZE)
        {
            message->code = DVM_DESK_RESIZE;
            message->size = 0;
            return;
        }
        if (code == FVM_MOUSE_WHEEL_UP || code == FVM_MOUSE_WHEEL_DOWN)
        {
            FVM_KeyboardState state;
            memset(&state, 0, sizeof(state));
            state.struct_size = sizeof(state);
            if (fvm_input_get_keyboard(context, &state) == FVM_OK && state.down[FVM_KEY_CONTROL])
            {
                code = code == FVM_MOUSE_WHEEL_UP ? 8 : 9;
            }
        }
        message->code = DVM_DESK_POINTER;
        message->first = message->first < 0 ? -1 : message->first / 20;
        message->second = message->second < 0 ? -1 : message->second / 20;
        if (code == 1 && !(header_drag && phase != FVM_INPUT_BEGIN))
        {
            if (vector_drag && phase != FVM_INPUT_BEGIN)
            {
                widget = vector_drag;
                if (!dvm_desktop_vector_point(widget, message->first, message->second, &position))
                {
                    vector_drag = widget = 0;
                    position = -1;
                }
            }
            else
            {
                dvm_desktop_hit(message->first, message->second, &widget, &position);
                if (phase == FVM_INPUT_BEGIN)
                {
                    long captured;
                    vector_drag =
                        position >= 0 && dvm_desktop_vector_point(widget, message->first, message->second, &captured)
                            ? widget
                            : 0;
                }
            }
        }
        if (code == 12)
        {
            dvm_desktop_hit(message->first, message->second, &widget, &position);
        }
        if (code == 4 || code == 5 || code == 8 || code == 9)
        {
            unsigned long tool;
            unsigned long color;
            unsigned long wide;
            dvm_desktop_hit(message->first, message->second, &widget, &position);
            if (position < 0 || !dvm_desktop_ink_style(widget, &tool, &color, &wide) || tool != 8)
            {
                widget = 0;
                position = -1;
            }
        }
        if (phase == FVM_INPUT_END || code != 1)
        {
            vector_drag = 0;
        }
        snprintf(metadata,
                 sizeof(metadata),
                 "%08lx%08lx%08lx%08lx",
                 phase,
                 code,
                 widget,
                 (unsigned long)position & 0xffffffffUL);
        memcpy(message->text, metadata, 32);
        message->size = 32;
        if (widget >= 1 && widget <= 8 && position >= 0)
        {
            message->id = 0x10000UL + dvm_desktop_vector_pick_radius(widget);
        }
        return;
    }
    if (message->code == EDITOR_RESIZE)
    {
        int rows = 0;
        char metadata[17];
        if (message->first >= EDITOR_WIDTH_MIN && message->first <= EDITOR_EXTENT_MAX &&
            message->second >= EDITOR_HEIGHT_MIN && message->second <= EDITOR_EXTENT_MAX)
        {
            rows = rows_for_canvas(font_size, (unsigned int)message->second);
        }
        message->kind = DVM_UI_REQUEST;
        message->id = 1;
        message->flags = view_revision;
        message->size = 16;
        snprintf(metadata, sizeof(metadata), "%08lx%08x", view_ticket, (unsigned int)rows);
        memcpy(message->text, metadata, 16);
        return;
    }
    {
        FVM_KeyboardState state;
        int wheel = message->code == FVM_MOUSE_WHEEL_UP || message->code == FVM_MOUSE_WHEEL_DOWN;
        int pinch = message->code == EDITOR_PINCH;
        int control;
        memset(&state, 0, sizeof(state));
        state.struct_size = sizeof(state);
        control = fvm_input_get_keyboard(context, &state) == FVM_OK && state.down[FVM_KEY_CONTROL];
        if ((wheel && control) || pinch)
        {
            int delta = pinch ? (message->first == 1    ? 1
                                 : message->first == -1 ? -1
                                                        : 0)
                              : (message->code == FVM_MOUSE_WHEEL_UP ? 1 : -1);
            int target = font_size + delta;
            int rows;
            if (target < EDITOR_FONT_MIN)
            {
                target = EDITOR_FONT_MIN;
            }
            if (target > EDITOR_FONT_MAX)
            {
                target = EDITOR_FONT_MAX;
            }
            rows = rows_for_canvas(target, canvas_height);
            if (!rows)
            {
                message->first = 0;
                message->second = 0;
            }
            else
            {
                message->first = delta;
                message->second = rows;
            }
            message->code = EDITOR_ZOOM;
        }
    }
    if (message->code == EDITOR_ZOOM)
    { /* Geometry was only measured above; Dis still validates and chooses its size. */
    }
    else if (message->code == FVM_MOUSE_WHEEL_UP || message->code == FVM_MOUSE_WHEEL_DOWN)
    {
        message->first = message->code == FVM_MOUSE_WHEEL_UP ? -3 : 3;
        message->second = 0;
        message->code = EDITOR_SCROLL;
    }
    else if (message->code == FVM_MOUSE_LEFT)
    {
        message->first = panel_mode ? field_hit((double)message->first / 20, (double)message->second / 20)
                                    : hit_position((double)message->first / 20, (double)message->second / 20);
        message->second = (long)message->flags;
        message->code = panel_mode ? EDITOR_FIELD_PLACE : EDITOR_PLACE;
    }
    else
    {
        return;
    }
    message->kind = DVM_UI_REQUEST;
    message->id = 1;
    message->flags = view_revision;
    message->size = 8;
    {
        char ticket[9];
        snprintf(ticket, sizeof(ticket), "%08lx", view_ticket);
        memcpy(message->text, ticket, 8);
    }
}


#ifdef DVM_EDITOR_SDL


/* Translate SDL virtual keys; zero means no supported Dis key, never synthesize text. */
static unsigned int keycode(SDL_Keycode key)
{
    if (key >= SDLK_a && key <= SDLK_z)
    {
        return (unsigned int)(key - SDLK_a + 'A');
    }
    switch (key)
    {
    case SDLK_TAB:
        return 9;
    case SDLK_RETURN:
        return FVM_KEY_ENTER;
    case SDLK_ESCAPE:
        return FVM_KEY_ESCAPE;
    case SDLK_F1:
        return FVM_KEY_F1;
    case SDLK_F7:
        return FVM_KEY_F1 + 6;
    case SDLK_F8:
        return FVM_KEY_F1 + 7;
    case SDLK_F3:
        return FVM_KEY_F1 + 2;
    case SDLK_BACKSPACE:
        return FVM_KEY_BACKSPACE;
    case SDLK_DELETE:
        return FVM_KEY_DELETE;
    case SDLK_LEFT:
        return FVM_KEY_LEFT;
    case SDLK_RIGHT:
        return FVM_KEY_RIGHT;
    case SDLK_UP:
        return FVM_KEY_UP;
    case SDLK_DOWN:
        return FVM_KEY_DOWN;
    case SDLK_PAGEUP:
        return FVM_KEY_PGUP;
    case SDLK_PAGEDOWN:
        return FVM_KEY_PGDN;
    case SDLK_HOME:
        return FVM_KEY_HOME;
    case SDLK_END:
        return FVM_KEY_END;
    case SDLK_LCTRL:
    case SDLK_RCTRL:
        return FVM_KEY_CONTROL;
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
        return FVM_KEY_SHIFT;
    default:
        return 0;
    }
}
#endif


/* Poll at most one event after the previous Dis view completes; never wait or edit text.
 * Native keyboard events do not wait for raster completion. Replay and pointer
 * events keep the pixel/geometry barrier; native desktop typing batches painting.
 * message receives a value only on one. Replay truncation/EOF without Quit returns minus one. */
int dvm_editor_event(DVM_UIMessage *message)
{
#include "event_prepare.inc"
#include "event_native.inc"

}

/* Tear down only after scheduler/channel routing is quiescent; null resources are allowed. */
void dvm_editor_destroy(void)
{
    dvm_editor_idle_reset();
    pixels_valid = 0;
    dvm_ink_reset();
#ifdef DVM_EDITOR_SDL
    postponed_valid = 0;
#endif
    ink_token = 0;
    header_drag = 0;
    desktop_pan = 0;
    dvm_desktop_local_cancel();
    vector_drag = 0; /* [GC:] Join all native callbacks before destroying fonts or either VM. */
    fvm_render_queue_destroy(raster_queue);
    raster_queue = NULL;
    dvm_editor_frame_destroy(displayed_frame);
    displayed_frame = NULL;
    upload_frame = NULL;
#ifdef DVM_EDITOR_SDL
    if (texture)
    {
        SDL_DestroyTexture(texture);
    }
    if (renderer)
    {
        SDL_DestroyRenderer(renderer);
    }
    if (window)
    {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
#endif
    if (events)
    {
        fclose(events);
    }
    if (panel_field && context)
    {
        fvm_gc_unpin(context, panel_field);
    }
    if (field && context)
    {
        fvm_gc_unpin(context, field);
    }
    fvm_text_layout_destroy(hit_layout);
    fvm_text_layout_destroy(panel_layout);
    fvm_font_destroy(layout_font);
    dvm_desktop_destroy();
#ifdef DVM_EDITOR_SDL
    desktop_animation_clock_set = 0;
#endif
    desktop_bound = 0;
    if (paint_callback)
    {
        fvm_gc_unpin(context, paint_callback);
        paint_callback = NULL;
    }
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    free(snapshot);
    free(clipboard_text);
    free(clipboard_transfer);
    free(pixels);
    free(font_bytes);
    free(default_font_path);
}
