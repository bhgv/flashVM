/* Copyright (c) 2026 bhgv
 * EN: Linux SDL2 fixture for the production paired desktop; posts real SDL events.
 * RU: Тестовые события SDL2 для штатного парного desktop в Linux.
 */
#define _GNU_SOURCE /* RTLD_NEXT for the fixture's one SDL polling interception. */
#include <SDL.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EVENT_DELAY_MS 650U /* Let the player mount, open and opt in before posting input. */
#define EDIT_ROW 106        /* First line of the standard paired editor. */
#define BUTTON_LEFT SDL_BUTTON_LEFT /* Native primary pointer. */


/* EN: Submit a copied event to SDL and fail the fixture if SDL discards it.
 * RU: Передать копию SDL и остановить проверку, если SDL отбросила событие. */
static void post(SDL_Event *event)
{
    if (SDL_PushEvent(event) != 1)
    {
        fprintf(stderr, "paired SDL fixture: %s\n", SDL_GetError());
        exit(2);
    }
}


/* EN: Submit virtual key, modifier and transition using the normal SDL queue.
 * RU: Передать клавишу, модификатор и переход через штатную очередь SDL. */
static void key(Uint32 type, SDL_Keycode code, Uint16 modifiers)
{
    SDL_Event event;
    memset(&event, 0, sizeof(event));
    event.type = type;
    event.key.state = type == SDL_KEYDOWN ? SDL_PRESSED : SDL_RELEASED;
    event.key.keysym.sym = code;
    event.key.keysym.mod = modifiers;
    post(&event);
}


/* EN: Submit one already committed UTF-8 payload as the OS text service would.
 * RU: Передать один подтверждённый пакет UTF-8, как оконная текстовая служба. */
static void text(const char *string)
{
    SDL_Event event;
    memset(&event, 0, sizeof(event));
    event.type = SDL_TEXTINPUT;
    SDL_strlcpy(event.text.text, string, sizeof(event.text.text));
    post(&event);
}


/* EN: Post one primary button transition at the editor's first row.
 * RU: Передать нажатие или отпускание основной кнопки в первой строке редактора. */
static void button(Uint32 type, int x)
{
    SDL_Event event;
    memset(&event, 0, sizeof(event));
    event.type = type;
    event.button.button = BUTTON_LEFT;
    event.button.state = (type == SDL_MOUSEBUTTONDOWN) ? SDL_PRESSED : SDL_RELEASED;
    event.button.x = x;
    event.button.y = EDIT_ROW;
    post(&event);
}


/* EN: Select a source range using the same button and motion sequence as a mouse.
 * RU: Выделить диапазон обычным нажатием, движением и отпусканием мыши. */
static void select_range(int first, int last)
{
    SDL_Event event;
    button(SDL_MOUSEBUTTONDOWN, first);
    memset(&event, 0, sizeof(event));
    event.type = SDL_MOUSEMOTION;
    event.motion.state = SDL_BUTTON_LMASK;
    event.motion.x = last;
    event.motion.y = EDIT_ROW;
    post(&event);
    button(SDL_MOUSEBUTTONUP, last);
}


/* EN: Type, replace a pointer-selected range, save, select again and close normally.
 * RU: Ввести текст, заменить выделенное мышью, сохранить, выделить ещё раз и закрыть. */
static void burst(void)
{
    SDL_Event event;
    text("Я🙂draft");
    key(SDL_KEYDOWN, SDLK_LSHIFT, KMOD_LSHIFT);
    memset(&event, 0, sizeof(event));
    event.type = SDL_WINDOWEVENT;
    event.window.event = SDL_WINDOWEVENT_FOCUS_LOST;
    post(&event);
    event.window.event = SDL_WINDOWEVENT_FOCUS_GAINED;
    post(&event);
    select_range(115, 150);
    text("paired");
    key(SDL_KEYDOWN, SDLK_LCTRL, KMOD_LCTRL);
    key(SDL_KEYDOWN, SDLK_s, KMOD_LCTRL);
    key(SDL_KEYUP, SDLK_s, KMOD_LCTRL);
    key(SDL_KEYUP, SDLK_LCTRL, KMOD_NONE);
    text("paired-output.txt");
    key(SDL_KEYDOWN, SDLK_RETURN, KMOD_NONE);
    key(SDL_KEYUP, SDLK_RETURN, KMOD_NONE);
    select_range(102, 115);
    key(SDL_KEYDOWN, SDLK_LCTRL, KMOD_LCTRL);
    memset(&event, 0, sizeof(event));
    event.type = SDL_MOUSEWHEEL;
    event.wheel.y = 1;
#if SDL_VERSION_ATLEAST(2, 0, 18)
    event.wheel.preciseY = 1;
#endif
#if SDL_VERSION_ATLEAST(2, 26, 0)
    event.wheel.mouseX = 200;
    event.wheel.mouseY = 160;
#endif
    post(&event);
    key(SDL_KEYUP, SDLK_LCTRL, KMOD_NONE);
    memset(&event, 0, sizeof(event));
    event.type = SDL_QUIT;
    post(&event);
}


/* EN: Let the actual SDL pump process everything, adding this burst once.
 * RU: Единожды добавить пачку и передать обработку штатному опросу SDL. */
int SDLCALL SDL_PollEvent(SDL_Event *event)
{
    static int (SDLCALL *poll_real)(SDL_Event *); /* Bound to SDL on the window owner. */
    static Uint32 first_poll; /* First poll time, plus one to reserve zero. */
    static int posted;        /* Prevent reentry while the burst is copied. */
    if (!poll_real)
    {
        void *symbol = dlsym(RTLD_NEXT, "SDL_PollEvent");
        memcpy(&poll_real, &symbol, sizeof(poll_real));
        if (!poll_real)
        {
            fputs("paired SDL fixture: SDL_PollEvent is unavailable\n", stderr);
            exit(2);
        }
    }
    if (!first_poll)
    {
        first_poll = SDL_GetTicks() + 1;
    }
    if (!posted && SDL_GetTicks() + 1 - first_poll >= EVENT_DELAY_MS && SDL_IsTextInputActive())
    {
        posted = 1;
        burst();
    }
    return poll_real(event);
}
