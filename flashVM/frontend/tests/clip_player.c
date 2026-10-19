/* Actual SDL player fixture: inject bounded input sequences after its first real presentation. */
#define fvm_desktop_window_events clip_player_real_events
#include "../common/sdl_window.c"
#undef fvm_desktop_window_events
static int injected; /* One entry batch per process; ordinary SDL/VM host code owns every object. */
/* Feed special/ASCII/repeat/unmapped keys through the actual SDL queue for KeyPress tests. */
static int inject_keypress(void)
{
    const SDL_Keycode keys[] = {SDLK_LEFT,SDLK_LEFT,SDLK_RIGHT,SDLK_HOME,SDLK_END,
        SDLK_INSERT,SDLK_DELETE,SDLK_BACKSPACE,SDLK_RETURN,SDLK_UP,SDLK_DOWN,
        SDLK_PAGEUP,SDLK_PAGEDOWN,SDLK_TAB,SDLK_a,SDLK_a,SDLK_2,SDLK_QUOTE,
        SDLK_SPACE,SDLK_F1,SDLK_LSHIFT};
    unsigned int i;
    for (i=0;i<sizeof(keys)/sizeof(keys[0]);++i)
    {
        SDL_Event event;
        memset(&event,0,sizeof(event));event.type=SDL_KEYDOWN;
        event.key.keysym.sym=keys[i];event.key.repeat=(Uint8)(i==1);
        if(i==14)event.key.keysym.mod=KMOD_SHIFT;
        if(i==15)event.key.keysym.mod=KMOD_SHIFT|KMOD_CAPS;
        if(i==16)event.key.keysym.mod=KMOD_SHIFT;
        if(SDL_PushEvent(&event)!=1)return 0;
        if(i==0)continue; /* The second Left is an autorepeat while the first remains held. */
        event.type=SDL_KEYUP;event.key.repeat=0;
        if(SDL_PushEvent(&event)!=1)return 0;
    }
    return 1;
}
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
        if (SDL_getenv("FLASHVM_TEST_KEYPRESS"))
        {
            if (!inject_keypress()) {view->event_error=FVM_ERROR;return 0;}
            return clip_player_real_events(view, timeout);
        }
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
