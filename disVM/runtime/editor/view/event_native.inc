/* Copyright (c) 2026 bhgv
 * EN: event_native.inc; private composition fragment, one owner translation unit.
 * RU: Частный фрагмент event_native.inc; единый translation unit владельца.
 */
#ifdef DVM_EDITOR_SDL
    {
        SDL_Event event;
        SDL_Event motion_points[DVM_EDITOR_MOTION_POINTS];
        unsigned int motion_count = 0;
        if (postponed_valid)
        {
            event = postponed_event;
            postponed_valid = 0;
        }
        else if (!SDL_PollEvent(&event))
        {
            return 0;
        }
        if (ink_token)
        {
            unsigned long widget;
            long point;
            if (!dvm_ink_last(ink_token, &widget, &point))
            {
                ink_token = 0;
                vector_drag = 0;
            }
        }
        /* SDL touch-generated mouse copies must not terminate their real finger.
         * Ignore foreign motion/releases instead of mapping them into this stroke. */
        if (((event.type == SDL_MOUSEMOTION && event.motion.which == SDL_TOUCH_MOUSEID) ||
             ((event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) &&
              event.button.which == SDL_TOUCH_MOUSEID)))
        {
            return 0;
        }
        int lost_pointer = 0;
        if (ink_token && (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONUP ||
                          event.type == SDL_FINGERMOTION || event.type == SDL_FINGERUP))
        {
            int ownership = dvm_editor_ink_pointer(&ink_pointer, &event);
            if (!ownership)
            {
                return 0;
            }
            lost_pointer = ownership < 0;
        }
        if (local_pointer_valid && (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONUP ||
            event.type == SDL_FINGERMOTION || event.type == SDL_FINGERUP))
        {
            int ownership = dvm_editor_ink_pointer(&local_pointer,&event);
            if (!ownership)
            {
                return 0;
            }
            if (ownership < 0)
            {
                lost_pointer = 1;
            }
        }
        if (dvm_desktop_local_active() && (lost_pointer || dvm_editor_motion_barrier(&event)))
        {
            int cancel = (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) ||
                (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) ||
                (event.type == SDL_FINGERDOWN);
            if (cancel)
            {
                dvm_desktop_local_cancel();
                local_pointer_valid = 0;
                vector_drag = 0;
                if (!request_paint())
                {
                    return -1;
                }
            }
            else
            {
                unsigned long widget;
                long point;
                if (dvm_desktop_local_position(&widget,&point))
                {
                    postponed_event = event;
                    postponed_valid = 1;
                    message->kind = DVM_UI_REQUEST;
                    message->id = 1;
                    message->code = DVM_DESK_POINTER;
                    message->flags = dvm_desktop_ticket();
                    snprintf(message->text,33,"%08x%08x%08lx%08lx",3,1,widget,(unsigned long)point);
                    message->size = 32;
                    vector_drag = 0;
                    local_pointer_valid = 0;
                    complete = 0;
                    return 1;
                }
            }
        }
        if (ink_token && (lost_pointer || dvm_editor_motion_barrier(&event)))
        {
            if ((event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) ||
                (event.type == SDL_FINGERDOWN && ink_pointer.type == SDL_FINGERDOWN &&
                 event.tfinger.windowID == ink_pointer.tfinger.windowID &&
                 (event.tfinger.touchId != ink_pointer.tfinger.touchId ||
                  event.tfinger.fingerId != ink_pointer.tfinger.fingerId)))
            {
                /* Added contacts convert a provisional canvas touch into a
                 * gesture. Cancel its native preview instead of committing a dot. */
                dvm_ink_cancel(ink_token);
                ink_token = 0;
                vector_drag = 0;
                request_paint();
            }
            else
            {
                unsigned long widget;
                long point;
                if (dvm_ink_last(ink_token, &widget, &point) && dvm_ink_end(ink_token, point))
                {
                    postponed_event = event;
                    postponed_valid = 1;
                    message->kind = DVM_UI_REQUEST;
                    message->id = 1;
                    message->code = DVM_DESK_POINTER;
                    message->flags = dvm_desktop_ticket();
                    snprintf(message->text,
                             41,
                             "%08x%08x%08x%08x%08x",
                             3,
                             1,
                             (unsigned int)widget,
                             (unsigned int)point,
                             (unsigned int)ink_token);
                    message->size = 40;
                    ink_token = 0;
                    vector_drag = 0;
                    complete = 0;
                    return 1;
                }
            }
        }
        if (dvm_desktop_active())
        {
            if (ink_token && (event.type == SDL_MOUSEMOTION || event.type == SDL_FINGERMOTION))
            {
                motion_count = dvm_editor_motion_batch(&event, motion_points);
            }
            else
            {
                unsigned long tool = 0;
                unsigned long color;
                unsigned long wide;
                int preserve = vector_drag &&
                    (!dvm_desktop_ink_style(vector_drag, &tool, &color, &wide) || tool == 1 || tool == 5);
                /* Primitives depend only on start/end. Keep the last queued
                 * endpoint, leaving release and control events in FIFO order. */
                dvm_editor_coalesce_motion(&event, preserve);
            }
        }
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_EXPOSED && window &&
            event.window.windowID == SDL_GetWindowID(window))
        {
            return present_pixels() ? 0 : -1; /* Re-present cached pixels without rebuilding either VM's view. */
        }
        if (dvm_editor_window_event(&event, window, message))
        {
            if (message->code == EDITOR_RESIZE)
            {
                header_drag = 0;
                desktop_pan = 0;
                vector_drag = 0;
                dvm_ink_cancel(ink_token);
                ink_token = 0;
                if (message->first == (long)canvas_width && message->second == (long)canvas_height)
                {
                    return 0;
                }
                pinch_distance = 0;
            }
        }
        else if (event.type == SDL_FINGERDOWN || event.type == SDL_FINGERUP || event.type == SDL_FINGERMOTION)
        {
            if (!dvm_desktop_active() || !desktop_touch(&event.tfinger, message))
            {
                pinch_distance = 0;
                return 0;
            }
        }
        else if (event.type == SDL_MULTIGESTURE)
        {
            if (dvm_desktop_active())
            {
                return 0; /* Desktop contacts already own this gesture. */
            }
            if (event.mgesture.numFingers < 2 || !(event.mgesture.dDist >= -1 && event.mgesture.dDist <= 1))
            {
                pinch_distance = 0;
                return 0;
            }
            pinch_distance += event.mgesture.dDist;
            if (fabs(pinch_distance) < EDITOR_PINCH_THRESHOLD)
            {
                return 0;
            }
            message->kind = DVM_UI_POINTER;
            message->code = EDITOR_PINCH;
            message->first = pinch_distance > 0 ? 1 : -1;
            pinch_distance = 0;
        }
        else if (event.type == SDL_MOUSEWHEEL)
        {
            if (!event.wheel.y)
            {
                return 0;
            }
            {
                int x;
                int y;
#if SDL_VERSION_ATLEAST(2, 26, 0)
                x = event.wheel.mouseX;
                y = event.wheel.mouseY;
#else
                SDL_GetMouseState(&x, &y);
#endif
                message->first = x * 20;
                message->second = y * 20;
            }
            if (dvm_desktop_active())
            {
                FVM_KeyboardState state;
                memset(&state, 0, sizeof(state));
                state.struct_size = sizeof(state);
                if (fvm_input_get_keyboard(context, &state) == FVM_OK && !state.down[FVM_KEY_CONTROL])
                {
                    unsigned int steps = dvm_editor_wheel_batch(&event);
                    message->flags = steps > 1 ? DVM_EDITOR_WHEEL_PHASE + steps : 0;
                }
            }
            message->kind = DVM_UI_POINTER;
            message->code = event.wheel.y > 0 ? FVM_MOUSE_WHEEL_UP : FVM_MOUSE_WHEEL_DOWN;
            if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
            {
                message->code = event.wheel.y > 0 ? FVM_MOUSE_WHEEL_DOWN : FVM_MOUSE_WHEEL_UP;
            }
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP ||
                 (event.type == SDL_MOUSEMOTION && (event.motion.state & (SDL_BUTTON_LMASK | SDL_BUTTON_MMASK))))
        {
            if ((event.type == SDL_MOUSEMOTION && event.motion.which == SDL_TOUCH_MOUSEID) ||
                (event.type != SDL_MOUSEMOTION && event.button.which == SDL_TOUCH_MOUSEID))
            {
                return 0;
            }
            if (!dvm_desktop_active() && event.type != SDL_MOUSEMOTION && event.button.button == SDL_BUTTON_MIDDLE)
            {
                return 0;
            }
            double x = event.type == SDL_MOUSEMOTION ? event.motion.x : event.button.x;
            double y = event.type == SDL_MOUSEMOTION ? event.motion.y : event.button.y;
            FVM_InputPhase phase = event.type == SDL_MOUSEBUTTONDOWN ? FVM_INPUT_BEGIN
                                   : event.type == SDL_MOUSEBUTTONUP ? FVM_INPUT_END
                                                                     : FVM_INPUT_UPDATE;
            if (event.type != SDL_MOUSEMOTION && event.button.button != SDL_BUTTON_LEFT &&
                event.button.button != SDL_BUTTON_MIDDLE)
            {
                return 0;
            }
            if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                desktop_pan = dvm_desktop_active() && dvm_editor_pan_button(event.button.button, SDL_GetModState());
                header_drag = !desktop_pan && dvm_desktop_active() && dvm_desktop_header(x, y);
            }
            if (fvm_input_set_pointer(context, x, y) != FVM_OK ||
                dvm_ui_capture_pointer(context, FVM_MOUSE_LEFT, phase, message) != FVM_OK)
            {
                return -1;
            }
            if (desktop_pan)
            {
                message->code = 7; /* Existing copied three-contact table-pan proposal. */
            }
            if (dvm_desktop_active() && event.type == SDL_MOUSEBUTTONDOWN &&
                event.button.clicks >= 2 && (event.button.clicks % 2) == 0 &&
                dvm_desktop_zoom_button(x,y))
            {
                message->code = 12;
            }
            if (header_drag && dvm_desktop_active() && event.type == SDL_MOUSEBUTTONDOWN && event.button.clicks >= 2 &&
                (event.button.clicks % 2) == 0)
            {
                message->code = 10;
            }
        }
        else if (event.type == SDL_TEXTINPUT)
        {
            if (!dvm_editor_text_batch(context, &event.text, message))
            {
                return -1;
            }
        }
        else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
        {
            unsigned int code = keycode(event.key.keysym.sym);
            if (!code && event.type == SDL_KEYDOWN &&
                (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_TAB))
            {
                message->kind = DVM_UI_TEXT;
                message->size = 1;
                message->text[0] = event.key.keysym.sym == SDLK_RETURN ? '\n' : '\t';
            }
            else if (code)
            {
                FVM_KeyboardState state;
                memset(&state, 0, sizeof(state));
                state.struct_size = sizeof(state);
                if (fvm_input_get_keyboard(context, &state) != FVM_OK)
                {
                    return -1;
                }
                state.down[code] = event.type == SDL_KEYDOWN;
                state.last_code = code;
                state.last_ascii = 0;
                if (fvm_input_set_keyboard(context, &state) != FVM_OK)
                {
                    return -1;
                }
                if (dvm_editor_plain_letter(&event.key))
                {
                    return 0; /* TextInput owns the edit; preserve native state without a no-op Dis view. */
                }
                if (dvm_ui_capture_keyboard(context, code, event.type == SDL_KEYDOWN, message) != FVM_OK)
                {
                    return -1;
                }
            }
            else
            {
                return 0;
            }
        }
        else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
        {
            header_drag = 0;
            desktop_pan = 0;
            vector_drag = 0;
            local_pointer_valid = 0;
            dvm_desktop_local_cancel();
            pinch_distance = 0;
            desktop_touch(NULL, message);
            if (fvm_input_reset_keyboard(context) != FVM_OK)
            {
                return -1;
            }
            message->kind = DVM_UI_FOCUS;
        }
        else
        {
            return 0;
        }
        resolve_pointer(message);
        if (vector_drag && !ink_token && message->kind == DVM_UI_REQUEST && message->code == DVM_DESK_POINTER &&
            message->size == 32 && (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_FINGERDOWN))
        {
            unsigned long tool;
            unsigned long color;
            unsigned long wide;
            unsigned long point;
            if (dvm_desktop_ink_style(vector_drag, &tool, &color, &wide) && tool <= 5 &&
                hexword(message->text + 24, &point))
            {
                if (tool == 4)
                {
                    long anchor;
                    if (dvm_desktop_line_anchor(vector_drag, &anchor))
                    {
                        point = (unsigned long)anchor;
                    }
                }
                ink_token = dvm_ink_begin(vector_drag, color, tool == 1 || tool == 5 ? wide : 1, (long)point);
                dvm_ink_tool(ink_token, tool);
                if (ink_token)
                {
                    ink_pointer = event;
                    snprintf(message->text + 32, 9, "%08x", (unsigned int)ink_token);
                    message->size = 40;
                }
            }
        }
        if (!events && message->kind == DVM_UI_REQUEST && message->code == DVM_DESK_POINTER &&
            message->size == 32)
        {
            unsigned long phase;
            unsigned long code;
            unsigned long widget;
            unsigned long encoded;
            if (hexword(message->text,&phase) && hexword(message->text+8,&code) &&
                hexword(message->text+16,&widget) && hexword(message->text+24,&encoded) && code == 1)
            {
                if (phase == 1 && vector_drag)
                {
                    unsigned long tool;
                    unsigned long color;
                    unsigned long wide;
                    if (dvm_desktop_ink_style(widget,&tool,&color,&wide) && (tool == 6 || tool == 7))
                    {
                        local_pointer = event;
                        local_pointer_valid = 1;
                        if (tool == 7 && !(SDL_GetModState() & KMOD_CTRL))
                        {
                            dvm_desktop_local_begin(widget,(long)encoded);
                        }
                    }
                }
                if (phase == 2 || phase == 3)
                {
                    long point = (long)encoded;
                    int local = dvm_desktop_local_motion(widget,&point);
                    if (local)
                    {
                        if (local == 2 && !request_paint())
                        {
                            return -1;
                        }
                        if (phase == 2)
                        {
                            return 0; /* Captured source/decorations update in FlashVM; no Dis motion roundtrip. */
                        }
                        snprintf(message->text+24,9,"%08lx",(unsigned long)point);
                        dvm_desktop_local_release();
                        local_pointer_valid = 0;
                    }
                }
            }
        }
        if (ink_token && motion_count)
        {
            unsigned int index;
            for (index = 0; index < motion_count; ++index)
            {
                long point;
                double x = event.type == SDL_MOUSEMOTION ? motion_points[index].motion.x
                                                         : motion_points[index].tfinger.x * canvas_width;
                double y = event.type == SDL_MOUSEMOTION ? motion_points[index].motion.y
                                                         : motion_points[index].tfinger.y * canvas_height;
                if (dvm_desktop_vector_point(vector_drag, x, y, &point))
                {
                    dvm_ink_point(ink_token, point);
                }
            }
            if (event.type == SDL_FINGERMOTION)
            {
                DVM_UIMessage ignored;
                desktop_touch(&motion_points[motion_count - 1].tfinger, &ignored);
            }
            if (!request_paint())
            {
                return -1;
            }
            return 0; /* C owns live points; no channel event or Limbo snapshot. */
        }
        if (ink_token && (event.type == SDL_MOUSEBUTTONUP || event.type == SDL_FINGERUP))
        {
            unsigned long point;
            if (message->size == 32 && hexword(message->text + 24, &point) && dvm_ink_end(ink_token, (long)point))
            {
                snprintf(message->text + 32, 9, "%08x", (unsigned int)ink_token);
                message->size = 40; /* Only a frozen opaque token crosses on release. */
            }
            else
            {
                dvm_ink_cancel(ink_token);
            }
            ink_token = 0;
        }
        if (event.type == SDL_MOUSEBUTTONUP || event.type == SDL_FINGERUP)
        {
            local_pointer_valid = 0;
        }
        if (event.type == SDL_MOUSEBUTTONUP)
        {
            desktop_pan = 0;
            header_drag = 0;
            vector_drag = 0;
        }
        complete = 0;
        return 1;
    }
#else
    return -1;
#endif
