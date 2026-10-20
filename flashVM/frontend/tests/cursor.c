/* Real SDL visibility query/application plus observed reverse-order host cleanup. */
#include <SDL.h>
static unsigned changes;
static int last_change;
static int observed_cursor(int value)
{
    if(value != SDL_QUERY) { ++changes; last_change=value; }
    return SDL_ShowCursor(value);
}
#define SDL_ShowCursor observed_cursor
#include "../common/sdl_window.c"
#undef SDL_ShowCursor
#include <assert.h>
int main(void)
{
    FVM_DesktopWindow *v; FVM_Config config; FVM_Runtime *r; FVM_Context *c;
    int previous, initial; unsigned before;
    assert(SDL_setenv("SDL_VIDEODRIVER","dummy",1)==0);
    v=fvm_desktop_window_open("cursor",32,32); assert(v);
    {
        SDL_Event e; memset(&e,0,sizeof(e)); e.type=SDL_WINDOWEVENT;
        e.window.event=SDL_WINDOWEVENT_ENTER; assert(handle_event(v,&e));
    }
    initial=SDL_ShowCursor(SDL_QUERY); before=changes;
    assert(fvm_config_init(&config)==FVM_OK && fvm_runtime_create(&config,&r)==FVM_OK);
    assert(fvm_context_create(r,&c)==FVM_OK);
    assert(fvm_input_set_cursor_visible(c,initial,&previous)==FVM_OK);
    assert(fvm_desktop_window_sync_cursor(v,c)==FVM_OK && changes==before);
    assert(fvm_input_set_cursor_visible(c,!initial,&previous)==FVM_OK);
    assert(fvm_desktop_window_sync_cursor(v,c)==FVM_OK && changes==before+1);
    assert(SDL_ShowCursor(SDL_QUERY)==!initial);
    assert(fvm_desktop_window_sync_cursor(v,c)==FVM_OK && changes==before+1);
    assert(fvm_input_set_cursor_visible(c,initial,&previous)==FVM_OK);
    v->background=1;
    assert(fvm_desktop_window_sync_cursor(v,c)==FVM_OK && changes==before+2);
    v->background=0;
    assert(fvm_desktop_window_sync_cursor(v,c)==FVM_OK && SDL_ShowCursor(SDL_QUERY)==initial);
    assert(fvm_desktop_window_sync_cursor(NULL,c)==FVM_INVALID_ARGUMENT);
    assert(fvm_desktop_window_sync_cursor(v,NULL)==FVM_INVALID_ARGUMENT);
    assert(fvm_input_set_cursor_visible(c,!initial,&previous)==FVM_OK);
    assert(fvm_desktop_window_sync_cursor(v,c)==FVM_OK);
    {
        SDL_Event e; memset(&e,0,sizeof(e)); e.type=SDL_WINDOWEVENT;
        e.window.event=SDL_WINDOWEVENT_LEAVE;
        assert(handle_event(v,&e) && SDL_ShowCursor(SDL_QUERY)==initial);
        assert(fvm_desktop_window_sync_cursor(v,c)==FVM_OK && SDL_ShowCursor(SDL_QUERY)==initial);
        e.window.event=SDL_WINDOWEVENT_ENTER;
        assert(handle_event(v,&e) && fvm_desktop_window_sync_cursor(v,c)==FVM_OK);
        assert(SDL_ShowCursor(SDL_QUERY)==!initial);
        e.type=SDL_APP_WILLENTERBACKGROUND;
        assert(handle_event(v,&e) && SDL_ShowCursor(SDL_QUERY)==initial);
    }
    fvm_desktop_window_close(v); assert(last_change==initial);
    fvm_context_destroy(c); fvm_runtime_destroy(r); return 0;
}
