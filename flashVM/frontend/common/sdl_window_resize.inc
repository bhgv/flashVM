/* Copyright (c) 2026 bhgv
 * EN: Host resize policy, ordered notifications and atomic texture extent replacement.
 * RU: Правила изменения окна, порядок уведомлений и атомарная замена размера текстуры.
 */


/* EN: Queue a bounded size, superseding only an adjacent unsent resize.
 * RU: Поставить допустимый размер в очередь, заменяя только соседнее событие изменения размера. */
static void queue_resize(FVM_DesktopWindow *view, int width, int height)
{
    FVM_DesktopEvent *event = NULL;
    if (width < (int)view->min_width || height < (int)view->min_height ||
        width > (int)view->max_width || height > (int)view->max_height)
    {
        return;
    }
    if (view->event_count)
    {
        size_t last = (view->event_head + view->event_count - 1) % FVM_DESKTOP_PENDING_EVENTS;
        if (view->pending_events[last].kind == FVM_DESKTOP_EVENT_RESIZE)
        {
            event = view->pending_events + last;
        }
    }
    if (!event)
    {
        event = queue_input(view, FVM_DESKTOP_EVENT_RESIZE, FVM_MOUSE_EVENT_MOVE, 0, 0);
    }
    if (event)
    {
        event->x = width;
        event->y = height;
    }
}


/* EN/RU: See window.h. / См. window.h. */
int fvm_desktop_window_follow_size(FVM_DesktopWindow *view, unsigned min_width, unsigned min_height,
    unsigned max_width, unsigned max_height)
{
    int width, height;
    if (!view || !view->window)
    {
        return 0;
    }
    if (view->min_width == min_width && view->min_height == min_height &&
        view->max_width == max_width && view->max_height == max_height)
    {
        return 1;
    }
    if ((min_width && (!view->direct_input || !min_height || max_width < min_width || max_height < min_height ||
                      max_width > FVM_DESKTOP_MAX_PIXELS || max_height > FVM_DESKTOP_MAX_PIXELS)) ||
        SDL_RenderSetLogicalSize(view->renderer, min_width ? 0 : (int)view->logical_width,
            min_width ? 0 : (int)view->logical_height) != 0)
    {
        return 0;
    }
    view->min_width = min_width;
    view->min_height = min_height;
    view->max_width = max_width;
    view->max_height = max_height;
    if (min_width)
    {
        SDL_SetWindowMinimumSize(view->window, 1, 1);
        SDL_SetWindowMaximumSize(view->window, (int)max_width, (int)max_height);
        SDL_SetWindowMinimumSize(view->window, (int)min_width, (int)min_height);
        SDL_GetWindowSize(view->window, &width, &height);
        if (width != (int)view->logical_width || height != (int)view->logical_height)
        {
            queue_resize(view, width, height);
        }
    }
    else
    {
        SDL_SetWindowMinimumSize(view->window, 1, 1);
        SDL_SetWindowMaximumSize(view->window, INT_MAX, INT_MAX);
    }
    return 1;
}


/* EN/RU: See window.h. / См. window.h. */
int fvm_desktop_window_canvas(FVM_DesktopWindow *view, unsigned width, unsigned height)
{
    SDL_Texture *replacement;
    if (!view || !view->min_width || !width || !height ||
        width > FVM_DESKTOP_MAX_PIXELS || height > FVM_DESKTOP_MAX_PIXELS)
    {
        return 0;
    }
    if (view->logical_width == width && view->logical_height == height)
    {
        return 1;
    }
    replacement = SDL_CreateTexture(view->renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
        (int)width, (int)height);
    if (!replacement || SDL_SetTextureBlendMode(replacement, SDL_BLENDMODE_NONE) != 0)
    {
        if (replacement)
        {
            SDL_DestroyTexture(replacement);
        }
        return 0;
    }
    SDL_DestroyTexture(view->texture);
    view->texture = replacement;
    free(view->last_frame);
    view->last_frame = NULL;
    view->last_frame_size = 0;
    view->logical_width = width;
    view->logical_height = height;
    view->texture_invalid = 0;
    view->ready = 0;
    return 1;
}
