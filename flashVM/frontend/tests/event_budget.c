/* Actual SDL queue: native event storms yield without dropping later close/input. */
#include <stdio.h>
#include <stdlib.h>
#include "../common/sdl_window.c"


int main(void)
{
    FVM_DesktopWindow *v;
    SDL_Event e;
    unsigned i;
    int before;
    if (!(SDL_setenv("SDL_VIDEODRIVER", "dummy", 1) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    v = fvm_desktop_window_open("event-budget", 32, 32);
    if (!(v))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
    memset(&e, 0, sizeof(e));
    e.type = SDL_USEREVENT;
    for (i = 0; i < FVM_DESKTOP_EVENT_BATCH * 3; ++i)
    {
        if (!(SDL_PushEvent(&e) == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    e.type = SDL_KEYDOWN;
    e.key.keysym.sym = SDLK_LEFT;
    if (!(SDL_PushEvent(&e) == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < 3; ++i)
    {
        before = SDL_PeepEvents(NULL, 0, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);
        if (!(before > 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_desktop_window_events(v, 10000)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(SDL_PeepEvents(NULL, 0, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) ==
              before - (int)FVM_DESKTOP_EVENT_BATCH))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!v->event_count))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_desktop_window_events(v, 0) && v->event_count == 1 && v->input.left))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&e, 0, sizeof(e));
    e.type = SDL_USEREVENT;
    for (i = 0; i < FVM_DESKTOP_EVENT_BATCH; ++i)
    {
        if (!(SDL_PushEvent(&e) == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    e.type = SDL_QUIT;
    if (!(SDL_PushEvent(&e) == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_window_events(v, 10000)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!fvm_desktop_window_events(v, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_window_status(v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_desktop_window_close(v);
    return 0;
}
