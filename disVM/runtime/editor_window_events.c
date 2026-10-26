/* Window buttons are normal owner events, never direct document mutations. */
#include "editor_window_events.h"
#include <string.h>


/* Decode SDL lifecycle events while retaining the ordinary checked Dis controller boundary.
 * event/window are borrowed, message is caller-owned. No source, renderer or channel is accessed.
 * Returns one for a valid close/size proposal; zero leaves message byte-for-byte unchanged. */
int dvm_editor_window_event(const SDL_Event *event, SDL_Window *window, DVM_UIMessage *message)
{
    DVM_UIMessage candidate;
    int width;
    int height;
    int close;
    if (!event || !message)
    {
        return 0;
    }
    close = event->type == SDL_QUIT;
    if (!close)
    {
        if (!window || event->type != SDL_WINDOWEVENT || event->window.windowID != SDL_GetWindowID(window))
        {
            return 0;
        }
        close = event->window.event == SDL_WINDOWEVENT_CLOSE;
    }
    memset(&candidate, 0, sizeof(candidate));
    if (close)
    {
        candidate.kind = DVM_UI_REQUEST;
        candidate.id = 1;
        candidate.code = 65539; /* Idehost.Quit: controller confirms dirty documents. */
        candidate.flags = (SDL_GetModState() & KMOD_SHIFT) != 0;
    }
    else
    {
        switch (event->window.event)
        {
        case SDL_WINDOWEVENT_RESIZED:
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            width = event->window.data1;
            height = event->window.data2;
            break;
        case SDL_WINDOWEVENT_MAXIMIZED:
        case SDL_WINDOWEVENT_RESTORED:
            SDL_GetWindowSize(window, &width, &height);
            break;
        default:
            return 0;
        }
        if (width <= 0 || height <= 0)
        {
            return 0;
        }
        candidate.kind = DVM_UI_POINTER;
        candidate.code = 65552; /* Idehost.Resize: resolve_pointer binds revision/ticket before sending. */
        candidate.first = width;
        candidate.second = height;
    }
    *message = candidate;
    return 1;
}
