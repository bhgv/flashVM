/* Copyright (c) 2026 bhgv
 * EN: event_prepare.inc; private composition fragment, one owner translation unit.
 * RU: Частный фрагмент event_prepare.inc; единый translation unit владельца.
 */
    unsigned delivered;
    if (fvm_timers_poll(context, &delivered) != FVM_OK || !collect_frame(0))
    {
        return -1;
    }
    if (!complete)
    {
#ifdef DVM_EDITOR_SDL
        SDL_Event next;
        int available;
        if (postponed_valid)
        {
            next = postponed_event;
            available = 1;
        }
        else
        {
            available = SDL_PeepEvents(&next, 1, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);
        }
        if (events || !dvm_desktop_local_ready() || available != 1 ||
            (next.type != SDL_MOUSEMOTION && next.type != SDL_FINGERMOTION))
        {
            return 0;
        }
#else
        return 0;
#endif
    }
#ifdef DVM_EDITOR_SDL
    if (!events && dvm_desktop_active())
    {
        Uint32 now = SDL_GetTicks();
        Uint32 elapsed = now - desktop_animation_clock;
        if (!desktop_animation_clock_set)
        {
            desktop_animation_clock_set = 1;
            desktop_animation_clock = now;
        }
        else if (elapsed >= 8)
        {
            desktop_animation_clock = now;
            if (dvm_desktop_advance(elapsed > 1000 ? 1000 : elapsed) && !request_paint())
            {
                return -1;
            }
        }
    }
    if (desktop_dirty && paint_due && !raster_pending &&
        ((!keyboard_waiting() && !paint_motion_waiting()) ||
         (Uint32)(SDL_GetTicks() - desktop_dirty_since) >=
             (ink_token ? EDITOR_INK_BATCH_MS : EDITOR_KEYBOARD_BATCH_MS)))
    {
        if (!(dvm_desktop_active() ? render_desktop() : render()))
        {
            return -1;
        }
        desktop_dirty = 0;
        paint_due = 0;
    }
    if (presentation_blocks_input())
#else
    if (raster_pending)
#endif
    {
        return 0; /* Never hit-test newer owner geometry against older displayed pixels. */
    }
    memset(message, 0, sizeof(*message));
    if (clipboard_event(message))
    {
        complete = 0;
        return 1;
    }
    if (events)
    {
        unsigned char length[4];
        unsigned long count;
        char storage[DVM_UI_HEADER_SIZE + DVM_UI_TEXT_MAX];
        FVM_Value value;
        if (fread(length, 1, 4, events) != 4)
        {
            fprintf(stderr, "editor replay ended before acknowledged Quit\n");
            return -1;
        }
        count = length[0] | ((unsigned long)length[1] << 8) | ((unsigned long)length[2] << 16) |
                ((unsigned long)length[3] << 24);
        if (count < DVM_UI_HEADER_SIZE || count > sizeof(storage) || fread(storage, 1, count, events) != count ||
            fvm_value_string_bytes(&value, storage, count) != FVM_OK || dvm_ui_decode(&value, message) != FVM_OK)
        {
            return -1;
        }
        if (message->kind == DVM_UI_KEY && message->code >= 1 && message->code < FVM_KEYBOARD_CODE_COUNT &&
            message->flags <= 1)
        {
            FVM_KeyboardState state;
            memset(&state, 0, sizeof(state));
            state.struct_size = sizeof(state);
            if (fvm_input_get_keyboard(context, &state) != FVM_OK)
            {
                return -1;
            }
            state.down[message->code] = message->flags != 0;
            state.last_code = (unsigned int)message->code;
            state.last_ascii = 0;
            if (fvm_input_set_keyboard(context, &state) != FVM_OK)
            {
                return -1;
            }
        }
        if (message->kind == DVM_UI_FOCUS && !message->flags)
        {
            pinch_distance = 0;
#ifdef DVM_EDITOR_SDL
            desktop_touch(NULL, message);
#endif
            if (fvm_input_reset_keyboard(context) != FVM_OK)
            {
                return -1;
            }
        }
        if (message->kind == DVM_UI_REQUEST && message->code == DVM_DESK_CLOCK)
        {
            if (message->first < 0 || message->first > 1000 || message->second != 0 || message->size)
            {
                return -1;
            }
            dvm_desktop_advance((unsigned int)message->first);
        }
        resolve_pointer(message);
        complete = 0;
        return 1;
    }
