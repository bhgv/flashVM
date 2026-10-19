/* Actual SDL player fixture: inject five SDL transitions only after its first real presentation. */
#define fvm_desktop_window_events clip_player_real_events
#include "../common/sdl_window.c"
#undef fvm_desktop_window_events
static int injected; /* One entry batch per process; ordinary SDL/VM host code owns every object. */
/* Feed SDL's real queue at a visible-scene boundary, then use the real drain/wait implementation. */
int fvm_desktop_window_events(FVM_DesktopWindow *view, int timeout)
{
    if (view && view->ready && !injected)
    {
        SDL_Event event;
        unsigned int i;
        const Uint32 types[] = {SDL_MOUSEMOTION, SDL_MOUSEBUTTONDOWN, SDL_MOUSEBUTTONUP,
                                SDL_KEYDOWN, SDL_KEYUP};
        injected = 1;
        for (i = 0; i < sizeof(types) / sizeof(types[0]); ++i)
        {
            memset(&event, 0, sizeof(event));
            event.type = types[i];
            if (i == 0) { event.motion.x = 10; event.motion.y = 20; }
            else if (i < 3)
            {
                event.button.button = SDL_BUTTON_LEFT;
                event.button.x = 10; event.button.y = 20;
            }
            else event.key.keysym.sym = SDLK_LEFT;
            if (SDL_PushEvent(&event) != 1)
            {
                view->event_error = FVM_ERROR;
                return 0;
            }
        }
    }
    return clip_player_real_events(view, timeout);
}
