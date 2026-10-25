/* Real SDL visibility query/application plus observed reverse-order host cleanup. */
#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>

static unsigned changes;
static int last_change;


static int observed_cursor(int value)
{
    if (value != SDL_QUERY)
    {
        ++changes;
        last_change = value;
    }
    return SDL_ShowCursor(value);
}

#define SDL_ShowCursor observed_cursor
#include "../common/sdl_window.c"

#undef SDL_ShowCursor


int main(void)
{
    FVM_DesktopWindow *v;
    FVM_Config config;
    FVM_Runtime *r;
    FVM_Context *c;
    int previous, initial;
    unsigned before;
    if (!(SDL_setenv("SDL_VIDEODRIVER", "dummy", 1) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    v = fvm_desktop_window_open("cursor", 32, 32);
    if (!(v))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    {
        SDL_Event e;
        memset(&e, 0, sizeof(e));
        e.type = SDL_WINDOWEVENT;
        e.window.event = SDL_WINDOWEVENT_ENTER;
        if (!(handle_event(v, &e)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    initial = SDL_ShowCursor(SDL_QUERY);
    before = changes;
    if (!(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &r) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(r, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_set_cursor_visible(c, initial, &previous) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_window_sync_cursor(v, c) == FVM_OK && changes == before))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_set_cursor_visible(c, !initial, &previous) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_window_sync_cursor(v, c) == FVM_OK && changes == before + 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(SDL_ShowCursor(SDL_QUERY) == !initial))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_window_sync_cursor(v, c) == FVM_OK && changes == before + 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_set_cursor_visible(c, initial, &previous) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    v->background = 1;
    if (!(fvm_desktop_window_sync_cursor(v, c) == FVM_OK && changes == before + 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    v->background = 0;
    if (!(fvm_desktop_window_sync_cursor(v, c) == FVM_OK && SDL_ShowCursor(SDL_QUERY) == initial))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_window_sync_cursor(NULL, c) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_window_sync_cursor(v, NULL) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_set_cursor_visible(c, !initial, &previous) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_window_sync_cursor(v, c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    {
        SDL_Event e;
        memset(&e, 0, sizeof(e));
        e.type = SDL_WINDOWEVENT;
        e.window.event = SDL_WINDOWEVENT_LEAVE;
        if (!(handle_event(v, &e) && SDL_ShowCursor(SDL_QUERY) == initial))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_desktop_window_sync_cursor(v, c) == FVM_OK && SDL_ShowCursor(SDL_QUERY) == initial))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        e.window.event = SDL_WINDOWEVENT_ENTER;
        if (!(handle_event(v, &e) && fvm_desktop_window_sync_cursor(v, c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(SDL_ShowCursor(SDL_QUERY) == !initial))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        e.type = SDL_APP_WILLENTERBACKGROUND;
        if (!(handle_event(v, &e) && SDL_ShowCursor(SDL_QUERY) == initial))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_desktop_window_close(v);
    if (!(last_change == initial))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(r);
    return 0;
}
