/* Copyright (c) 2026 bhgv
 * EN: Inject real SDL queue events after measured quiet intervals, only in a private test link.
 * RU: Ввод настоящих событий SDL после измеренных тихих интервалов в частной тестовой сборке.
 */
#define _POSIX_C_SOURCE 200809L
#include <SDL.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static int phase; /* UI-main-only test phase, never shared with a guest. */
static double start; /* First event-poll monotonic time in seconds. */
static double cpu_start; /* Process CPU sample across all threads. */
static double quiet_start; /* Matching monotonic sample boundary. */

int __real_SDL_PollEvent(SDL_Event *event); /* Original pinned SDL implementation. */


/* EN/RU: Read clock id in seconds; return negative on failure, without VM access.
 * Прочитать часы id в секундах; при ошибке вернуть отрицательное число. */
static double seconds(clockid_t id)
{
    struct timespec value;
    if (clock_gettime(id, &value))
    {
        return -1;
    }
    return (double)value.tv_sec + (double)value.tv_nsec / 1000000000.0;
}


/* EN/RU: Publish a quiet-interval CPU observation named label; no guest state.
 * Вывести CPU за тихий интервал label; состояние гостя не затрагивается. */
static void measured(const char *label)
{
    double elapsed = seconds(CLOCK_MONOTONIC) - quiet_start;
    double used = seconds(CLOCK_PROCESS_CPUTIME_ID) - cpu_start;
    fprintf(stderr, "SDLIDLE %s cpu=%.2f elapsed=%.2f\n", label, 100 * used / elapsed, elapsed);
}


/* EN/RU: Queue a copied window event kind for borrowed window; no direct Dis calls.
 * Поставить копию события kind для window; прямых вызовов Dis нет. */
static void lifecycle(SDL_Window *window, Uint8 kind)
{
    SDL_Event event;
    memset(&event, 0, sizeof(event));
    event.type = SDL_WINDOWEVENT;
    event.window.windowID = SDL_GetWindowID(window);
    event.window.event = kind;
    SDL_PushEvent(&event);
}


/* EN/RU: Queue key type/symbol/modifier values for borrowed window, keeping FIFO order.
 * Поставить key type/symbol/modifiers для window в порядке FIFO. */
static void key(SDL_Window *window, Uint32 type, SDL_Keycode symbol, Uint16 modifiers)
{
    SDL_Event event;
    memset(&event, 0, sizeof(event));
    event.type = type;
    event.key.windowID = SDL_GetWindowID(window);
    event.key.state = type == SDL_KEYDOWN ? SDL_PRESSED : SDL_RELEASED;
    event.key.keysym.sym = symbol;
    event.key.keysym.mod = modifiers;
    SDL_PushEvent(&event);
}


/* EN: Intercept only native event polling on UI main. event belongs to SDL's
 * caller; return the real queue result after injecting scheduled lifecycle,
 * text/save and Quit events. All production rendering and Dis code is unchanged.
 * RU: Перехватить только опрос событий в UI main. event принадлежит вызывающему;
 * вернуть результат настоящей очереди после ввода lifecycle, текста/save и Quit.
 * Производственная отрисовка и код Dis не изменяются. */
int __wrap_SDL_PollEvent(SDL_Event *event)
{
    double now = seconds(CLOCK_MONOTONIC);
    SDL_Window *window = SDL_GetWindowFromID(1);
    if (!start)
    {
        start = now;
    }
    if (window && phase == 0 && now - start >= 1)
    {
        quiet_start = now;
        cpu_start = seconds(CLOCK_PROCESS_CPUTIME_ID);
        phase = 1;
    }
    else if (window && phase == 1 && now - start >= 3)
    {
        measured("visible");
        SDL_MinimizeWindow(window);
        lifecycle(window, SDL_WINDOWEVENT_MINIMIZED);
        quiet_start = now;
        cpu_start = seconds(CLOCK_PROCESS_CPUTIME_ID);
        phase = 2;
    }
    else if (window && phase == 2 && now - start >= 5)
    {
        measured("minimized");
        SDL_RestoreWindow(window);
        lifecycle(window, SDL_WINDOWEVENT_RESTORED);
        SDL_SetWindowSize(window, 1000, 700);
        lifecycle(window, SDL_WINDOWEVENT_FOCUS_GAINED);
        phase = 3;
    }
    else if (window && phase == 3 && now - start >= 5.5)
    {
        SDL_Event text;
        memset(&text, 0, sizeof(text));
        text.type = SDL_TEXTINPUT;
        text.text.windowID = SDL_GetWindowID(window);
        strcpy(text.text.text, "native-");
        SDL_PushEvent(&text);
        phase = 4;
    }
    else if (window && phase == 4 && now - start >= 6)
    {
        SDL_SetModState(KMOD_CTRL);
        key(window, SDL_KEYDOWN, SDLK_LCTRL, KMOD_CTRL);
        key(window, SDL_KEYDOWN, SDLK_s, KMOD_CTRL);
        key(window, SDL_KEYUP, SDLK_s, KMOD_CTRL);
        key(window, SDL_KEYUP, SDLK_LCTRL, KMOD_NONE);
        phase = 5;
    }
    else if (phase == 5 && now - start >= 6.5)
    {
        SDL_SetModState(KMOD_NONE);
        phase = 6;
    }
    else if (phase == 6 && now - start >= 7)
    {
        SDL_Event quit;
        memset(&quit, 0, sizeof(quit));
        quit.type = SDL_QUIT;
        SDL_SetModState(KMOD_SHIFT);
        SDL_PushEvent(&quit);
        phase = 7;
    }
    return __real_SDL_PollEvent(event);
}
