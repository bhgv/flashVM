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
#define MDI_WIDTH 860       /* One-column desktop width exercised in the resize test. */
#define MDI_HEIGHT 620      /* New native window height during the MDI test. */


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
static void button(Uint32 type, int x, int y)
{
    SDL_Event event;
    memset(&event, 0, sizeof(event));
    event.type = type;
    event.button.button = BUTTON_LEFT;
    event.button.state = (type == SDL_MOUSEBUTTONDOWN) ? SDL_PRESSED : SDL_RELEASED;
    event.button.x = x;
    event.button.y = y;
    post(&event);
}


/* EN: Select a source range using the same button and motion sequence as a mouse.
 * RU: Выделить диапазон обычным нажатием, движением и отпусканием мыши. */
static void select_range(int first, int last)
{
    SDL_Event event;
    button(SDL_MOUSEBUTTONDOWN, first, EDIT_ROW);
    memset(&event, 0, sizeof(event));
    event.type = SDL_MOUSEMOTION;
    event.motion.state = SDL_BUTTON_LMASK;
    event.motion.x = last;
    event.motion.y = EDIT_ROW;
    post(&event);
    button(SDL_MOUSEBUTTONUP, last, EDIT_ROW);
}


/* EN: Post a complete primary click at a visible pane or tab in window pixels.
 * RU: Передать полный щелчок по видимой панели или вкладке в пикселях окна. */
static void click(int x, int y)
{
    button(SDL_MOUSEBUTTONDOWN, x, y);
    button(SDL_MOUSEBUTTONUP, x, y);
}


/* EN: Post one Ctrl shortcut with clean release before the next visible action.
 * RU: Передать сочетание Ctrl с отпусканием перед следующим действием. */
static void shortcut(SDL_Keycode code)
{
    key(SDL_KEYDOWN, SDLK_LCTRL, KMOD_LCTRL);
    key(SDL_KEYDOWN, code, KMOD_LCTRL);
    key(SDL_KEYUP, SDLK_LCTRL, KMOD_NONE);
}


/* EN: Save the active dirty document under a fresh relative name.
 * RU: Сохранить активный изменённый документ под новым относительным именем. */
static void save(const char *name)
{
    shortcut(SDLK_s);
    text(name);
    key(SDL_KEYDOWN, SDLK_RETURN, KMOD_NONE);
}


/* EN: Resize the actual dummy SDL window and exercise MDI owners on both sides of the change.
 * RU: Изменить размер настоящего окна SDL dummy и проверить MDI до и после перестройки. */
static void mdi_burst(void)
{
    SDL_Window *window = SDL_GetWindowFromID(1);
    SDL_Event event;
    int width, height;
    if (!window)
    {
        fputs("paired SDL fixture: no native window to resize\n", stderr);
        exit(2);
    }
    text("Первый");
    shortcut(SDLK_g);
    click(520, 520);
    text("Второй");
    shortcut(SDLK_g);
    SDL_SetWindowSize(window, MDI_WIDTH, MDI_HEIGHT);
    SDL_GetWindowSize(window, &width, &height);
    if (width != MDI_WIDTH || height != MDI_HEIGHT)
    {
        fputs("paired SDL fixture: native window size did not change\n", stderr);
        exit(2);
    }
    click(132, 112);
    click(110, 164);
    text("1-");
    save("paired-first.txt");
    click(450, 112);
    click(110, 164);
    text("2-");
    shortcut(SDLK_d);
    save("paired-second.txt");
    shortcut(SDLK_g);
    click(132, 112);
    memset(&event, 0, sizeof(event));
    event.type = SDL_QUIT;
    post(&event);
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
        if (getenv("FVM_PAIRED_TEST_MDI"))
        {
            mdi_burst();
        }
        else
        {
            burst();
        }
    }
    return poll_real(event);
}
