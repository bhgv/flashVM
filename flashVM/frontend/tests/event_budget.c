/* Actual SDL queue: native event storms yield without dropping later close/input. */
#include "../common/sdl_window.c"
#include <assert.h>
int main(void)
{
    FVM_DesktopWindow *v; SDL_Event e; unsigned i; int before;
    assert(SDL_setenv("SDL_VIDEODRIVER","dummy",1)==0);
    v=fvm_desktop_window_open("event-budget",32,32);assert(v);
    SDL_FlushEvents(SDL_FIRSTEVENT,SDL_LASTEVENT);
    memset(&e,0,sizeof(e));e.type=SDL_USEREVENT;
    for(i=0;i<FVM_DESKTOP_EVENT_BATCH*3;++i)assert(SDL_PushEvent(&e)==1);
    e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_LEFT;
    assert(SDL_PushEvent(&e)==1);
    for(i=0;i<3;++i)
    {
        before=SDL_PeepEvents(NULL,0,SDL_PEEKEVENT,SDL_FIRSTEVENT,SDL_LASTEVENT);
        assert(before>0);
        assert(fvm_desktop_window_events(v,10000));
        assert(SDL_PeepEvents(NULL,0,SDL_PEEKEVENT,SDL_FIRSTEVENT,SDL_LASTEVENT)==
               before-(int)FVM_DESKTOP_EVENT_BATCH);
        assert(!v->event_count);
    }
    assert(fvm_desktop_window_events(v,0) && v->event_count==1 && v->input.left);
    memset(&e,0,sizeof(e));e.type=SDL_USEREVENT;
    for(i=0;i<FVM_DESKTOP_EVENT_BATCH;++i)assert(SDL_PushEvent(&e)==1);
    e.type=SDL_QUIT;assert(SDL_PushEvent(&e)==1);
    assert(fvm_desktop_window_events(v,10000));
    assert(!fvm_desktop_window_events(v,0));
    assert(fvm_desktop_window_status(v)==FVM_OK);
    fvm_desktop_window_close(v);return 0;
}
