/* Exercise actual SDL queue events and a real dummy-driver window, not IDE replay envelopes. */
#include "editor_window_events.h"
#include <stdio.h>
#include <string.h>


/* Initialize a real SDL dummy window and check owner-filtered lifecycle translation.
 * No arguments. Returns nonzero on SDL/setup/admission failure; all owned SDL state is released. */
int main(void)
{
    SDL_Window *window;
    SDL_Event event;
    DVM_UIMessage message;
    DVM_UIMessage before;
    unsigned int types[] = {SDL_WINDOWEVENT_CLOSE, SDL_WINDOWEVENT_RESIZED, SDL_WINDOWEVENT_SIZE_CHANGED,
                            SDL_WINDOWEVENT_MAXIMIZED, SDL_WINDOWEVENT_RESTORED};
    size_t index;
    int status = 0;
    int attempt;
    int found;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        fprintf(stderr, "SDL test init: %s\n", SDL_GetError());
        return 1;
    }
    window = SDL_CreateWindow("event regression", 0, 0, 800, 600, SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        SDL_Quit();
        return 1;
    }
    while (SDL_PollEvent(&event))
    {
        /* Startup notifications are unrelated to the explicitly queued regression cases. */
    }
    for (index = 0; index < sizeof(types) / sizeof(types[0]); index++)
    {
        memset(&event, 0, sizeof(event));
        event.type = SDL_WINDOWEVENT;
        event.window.windowID = SDL_GetWindowID(window);
        event.window.event = types[index];
        event.window.data1 = 1000;
        event.window.data2 = 700;
        if (SDL_PushEvent(&event) != 1)
        {
            status = 1;
            break;
        }
        found = 0;
        for (attempt = 0; attempt < 32; attempt++)
        {
            if (SDL_PollEvent(&event) && event.type == SDL_WINDOWEVENT &&
                event.window.windowID == SDL_GetWindowID(window) && event.window.event == types[index])
            {
                found = 1;
                break;
            }
        }
        if (!found || !dvm_editor_window_event(&event, window, &message))
        {
            fprintf(stderr, "SDL window event missing/rejected: index=%lu\n", (unsigned long)index);
            status = 1;
            break;
        }
        if (index == 0 ? message.kind != DVM_UI_REQUEST || message.code != 65539
                       : message.kind != DVM_UI_POINTER || message.code != 65552 ||
                             message.first != (index < 3 ? 1000 : 800) ||
                             message.second != (index < 3 ? 700 : 600))
        {
            status = 1;
            break;
        }
    }
    memset(&message, 0xa5, sizeof(message));
    before = message;
    event.window.windowID = SDL_GetWindowID(window) + 1;
    if (dvm_editor_window_event(&event, window, &message) || memcmp(&message, &before, sizeof(message)))
    {
        status = 1;
    }
    event.window.windowID = SDL_GetWindowID(window);
    event.window.event = SDL_WINDOWEVENT_SIZE_CHANGED;
    event.window.data1 = 0;
    event.window.data2 = 0;
    if (dvm_editor_window_event(&event, window, &message) || memcmp(&message, &before, sizeof(message)))
    {
        status = 1;
    }
    memset(&event, 0, sizeof(event));
    event.type = SDL_QUIT;
    SDL_SetModState(KMOD_NONE);
    if (!dvm_editor_window_event(&event, window, &message) || message.code != 65539 || message.flags)
    {
        status = 1;
    }
    SDL_SetModState(KMOD_SHIFT);
    if (!dvm_editor_window_event(&event, window, &message) || message.flags != 1)
    {
        status = 1;
    }
    SDL_SetModState(KMOD_NONE);
    SDL_DestroyWindow(window);
    SDL_Quit();
    puts(status ? "FAIL SDL window lifecycle" : "PASS SDL queue: window close, resized, size-changed, maximize, restore, foreign filter");
    return status;
}
