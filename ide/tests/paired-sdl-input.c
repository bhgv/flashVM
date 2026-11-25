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


/* EN: Drag within one continuous physical gesture so the layout owns docking and shelf reorder.
 * RU: Передать непрерывный жест, чтобы компоновка обработала стыковку или порядок полки. */
static void drag(int from_x, int from_y, int to_x, int to_y)
{
    SDL_Event event;
    button(SDL_MOUSEBUTTONDOWN, from_x, from_y);
    memset(&event, 0, sizeof(event));
    event.type = SDL_MOUSEMOTION;
    event.motion.state = SDL_BUTTON_LMASK;
    event.motion.x = to_x;
    event.motion.y = to_y;
    post(&event);
    button(SDL_MOUSEBUTTONUP, to_x, to_y);
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


/* EN: Dock a real MDI group, reveal both source owners via their live cards and reorder a card.
 * RU: Свернуть настоящую группу MDI, открыть оба документа с полки и переставить карточку. */
static void thumbnails_burst(void)
{
    SDL_Event event;
    text("первая");
    key(SDL_KEYDOWN, SDLK_RETURN, KMOD_NONE);
    text("строка один");
    shortcut(SDLK_g);
    click(520, 520);
    text("вторая");
    key(SDL_KEYDOWN, SDLK_RETURN, KMOD_NONE);
    text("строка два");
    shortcut(SDLK_g);
    drag(650, 85, 30, 85);
    click(40, 100);
    click(110, 164);
    text("1-");
    save("paired-thumb-first.txt");
    click(927, 612);
    click(40, 520);
    click(110, 164);
    text("2-");
    save("paired-thumb-second.txt");
    drag(40, 520, 40, 362);
    click(40, 100);
    memset(&event, 0, sizeof(event));
    event.type = SDL_QUIT;
    post(&event);
}


/* EN: Open the prepared SVG through the ordinary shortcuts and Open dialog.
 * RU: Открыть подготовленный SVG штатными сочетаниями и диалогом открытия. */
static void open_vector(void)
{
    key(SDL_KEYDOWN, SDLK_LCTRL, KMOD_LCTRL);
    key(SDL_KEYDOWN, SDLK_LSHIFT, KMOD_LCTRL | KMOD_LSHIFT);
    key(SDL_KEYDOWN, SDLK_n, KMOD_LCTRL | KMOD_LSHIFT);
    key(SDL_KEYUP, SDLK_n, KMOD_LCTRL | KMOD_LSHIFT);
    key(SDL_KEYUP, SDLK_LSHIFT, KMOD_LCTRL);
    key(SDL_KEYUP, SDLK_LCTRL, KMOD_NONE);
    shortcut(SDLK_o);
    text("paired-drawing.svg");
    key(SDL_KEYDOWN, SDLK_RETURN, KMOD_NONE);
}


/* EN: Add a rectangle with the ordinary tool key, then save the active SVG.
 * RU: Добавить прямоугольник штатной клавишей и сохранить активный SVG. */
static void vector_rectangle(void)
{
    key(SDL_KEYDOWN, SDLK_r, KMOD_NONE);
    drag(230, 495, 365, 565);
    shortcut(SDLK_s);
}


/* EN: Open and save an SVG at its initial camera, then close the window.
 * RU: Открыть и сохранить SVG при исходной камере, затем закрыть окно. */
static void vector_burst(void)
{
    SDL_Event event;
    open_vector();
    vector_rectangle();
    memset(&event, 0, sizeof(event));
    event.type = SDL_QUIT;
    post(&event);
}


/* EN: Cut across the red fill with a brush, then move the blue stroke away.
 * RU: Разрезать красный контур кистью, затем отодвинуть синий мазок. */
static void merge_drawing_burst(void)
{
    SDL_Event event;
    open_vector();
    text("++++++++");
    drag(111, 470, 225, 470);
    key(SDL_KEYDOWN, SDLK_v, KMOD_NONE);
    drag(170, 470, 170, 569);
    shortcut(SDLK_s);
    memset(&event, 0, sizeof(event));
    event.type = SDL_QUIT;
    post(&event);
}


/* EN: Post a positive wheel step at the current window position in pixels.
 * RU: Передать положительный шаг колеса в указанной точке окна. */
static void vector_wheel(int x, int y)
{
    SDL_Event event;
    memset(&event, 0, sizeof(event));
    event.type = SDL_MOUSEMOTION;
    event.motion.x = x;
    event.motion.y = y;
    post(&event);
    memset(&event, 0, sizeof(event));
    event.type = SDL_MOUSEWHEEL;
    event.wheel.y = 1;
#if SDL_VERSION_ATLEAST(2, 0, 18)
    event.wheel.preciseY = 1;
#endif
#if SDL_VERSION_ATLEAST(2, 26, 0)
    event.wheel.mouseX = x;
    event.wheel.mouseY = y;
#endif
    post(&event);
}


/* EN: Use wheel and marquee, draw while zoomed, then reset and repeat the drag.
 * Leave the last zoom visible in the raster without a final document change.
 * RU: Увеличить колесом и рамкой, нарисовать, сбросить масштаб и повторить жест.
 * Оставить последнее увеличение на растре без изменения документа. */
static void vector_camera_burst(void)
{
    SDL_Event event;
    open_vector();
    click(403, 397);
    vector_wheel(235, 480);
    drag(220, 465, 366, 565);
    vector_rectangle();
    click(425, 397);
    vector_rectangle();
    click(403, 397);
    vector_wheel(169, 475);
    vector_wheel(169, 475);
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


/* EN: Edit the pinned startup document, save in place and request orderly exit.
 * RU: Отредактировать стартовый документ, сохранить на месте и штатно выйти. */
static void startup_document_burst(void)
{
    SDL_Event event;
    text("menu-");
    shortcut(SDLK_s);
    memset(&event, 0, sizeof(event));
    event.type = SDL_QUIT;
    post(&event);
}


#include "paired-sdl-ink.inc"


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
        if (getenv("FVM_PAIRED_TEST_DOCUMENT"))
        {
            startup_document_burst();
        }
        else if (getenv("FVM_PAIRED_TEST_THUMBNAILS"))
        {
            thumbnails_burst();
        }
        else if (getenv("FVM_PAIRED_TEST_MDI"))
        {
            mdi_burst();
        }
        else if (getenv("FVM_PAIRED_TEST_VECTOR"))
        {
            vector_burst();
        }
        else if (getenv("FVM_PAIRED_TEST_VECTOR_CAMERA"))
        {
            vector_camera_burst();
        }
        else if (getenv("FVM_PAIRED_TEST_LOCAL_INK"))
        {
            local_ink_burst();
        }
        else if (getenv("FVM_PAIRED_TEST_MERGE_DRAWING"))
        {
            merge_drawing_burst();
        }
        else if (getenv("FVM_PAIRED_TEST_BRUSH_SWEEP"))
        {
            sweep_burst();
        }
        else
        {
            burst();
        }
    }
    return poll_real(event);
}
