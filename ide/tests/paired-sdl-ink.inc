/* Copyright (c) 2026 bhgv
 * EN: Verify local pixels while the fixture's own Dis child cannot send a scene.
 * RU: Проверить локальные пиксели, пока дочерний Dis теста не может прислать сцену.
 */
#include <signal.h>
#include <unistd.h>

static pid_t ink_dis;        /* Verified child in this fixture's process group. */
static unsigned ink_phase;  /* Zero until requested; 1..6 track presented transitions. */
static unsigned ink_steps;  /* Samples posted after the second stroke becomes visible. */
static unsigned sweep_phase; /* Preview, timed release, Undo, Redo, then save. */
static Uint32 sweep_time;    /* Explicit SDL time at the test stroke's press. */


/* EN: Resume the child on normal completion or an explicit fixture failure.
 * RU: Возобновить дочерний Dis при завершении или явной ошибке теста. */
static void resume_ink_dis(void)
{
    if (ink_dis > 1)
    {
        kill(ink_dis, SIGCONT);
    }
}


/* EN: Borrow the SDL renderer and sample one pixel from its pending frame.
 * RU: Прочитать пиксель готовящегося кадра из заимствованного SDL-рендерера. */
static unsigned ink_pixel(SDL_Renderer *renderer, int x, int y)
{
    SDL_Rect pixel = {x, y, 1, 1};
    unsigned char rgb[3];
    if (SDL_RenderReadPixels(renderer, &pixel, SDL_PIXELFORMAT_RGB24, rgb, sizeof(rgb)) != 0)
    {
        fprintf(stderr, "paired ink capture: %s\n", SDL_GetError());
        exit(2);
    }
    return ((unsigned)rgb[0] << 16) | ((unsigned)rgb[1] << 8) | rgb[2];
}


/* EN: Queue exactly one held-button movement between presented frames.
 * RU: Поставить ровно одно движение с зажатой кнопкой между показанными кадрами. */
static void ink_motion(int x, int y)
{
    SDL_Event event;
    memset(&event, 0, sizeof(event));
    event.type = SDL_MOUSEMOTION;
    event.motion.state = SDL_BUTTON_LMASK;
    event.motion.x = x;
    event.motion.y = y;
    post(&event);
}


/* EN: Resolve the child written by the Python fixture and open its prepared SVG.
 * RU: Определить дочерний Dis из Python-теста и открыть подготовленный SVG. */
static void local_ink_burst(void)
{
    FILE *identity = fopen("paired-ink-dis.pid", "r");
    int pid = 0;
    if (!identity || fscanf(identity, "%d", &pid) != 1 || pid <= 1 || getpgid(pid) != getpgrp())
    {
        fputs("paired ink fixture: missing Dis child\n", stderr);
        exit(2);
    }
    fclose(identity);
    ink_dis = pid;
    atexit(resume_ink_dis);
    ink_phase = 1;
    open_vector();
}


/* EN: Suspend only the verified fixture child. The player still owns SDL input.
 * RU: Приостановить только проверенный дочерний процесс. Ввод SDL остаётся у плеера. */
static void suspend_ink_dis(void)
{
    if (kill(ink_dis, SIGSTOP) != 0)
    {
        perror("paired ink stop");
        exit(2);
    }
}


/* EN: Inspect real displayed pixels before release. Cancel the first gesture
 * while Dis is suspended. Send the second only after more than one wire batch
 * of distinct motions has been presented by FlashVM alone.
 * RU: Проверить показанные пиксели до отпускания. Отменить первый жест при
 * остановленном Dis. Отправить второй, когда FlashVM покажет больше одного
 * фрагмента отдельных движений. */
static void inspect_ink_frame(SDL_Renderer *renderer)
{
    SDL_Event event;
    if (ink_phase == 1 && ink_pixel(renderer, 170, 466) == 0xe04444)
    {
        suspend_ink_dis();
        ink_phase = 2;
        button(SDL_MOUSEBUTTONDOWN, 235, 490);
        ink_motion(276, 490);
    }
    else if (ink_phase == 2 && ink_pixel(renderer, 255, 490) == 0x245ac4)
    {
        ink_phase = 3;
        key(SDL_KEYDOWN, SDLK_ESCAPE, KMOD_NONE);
    }
    else if (ink_phase == 3 && ink_pixel(renderer, 255, 490) == 0xffffff)
    {
        ink_phase = 4;
        resume_ink_dis();
        puts("PASS local ink drew and canceled while Dis was suspended");
        button(SDL_MOUSEBUTTONUP, 276, 490);
        button(SDL_MOUSEBUTTONDOWN, 235, 518);
        ink_motion(276, 518);
    }
    else if (ink_phase == 4 && ink_pixel(renderer, 255, 518) == 0x245ac4)
    {
        suspend_ink_dis();
        ink_phase = 5;
        ink_motion(277, 518);
    }
    else if (ink_phase == 5)
    {
        if (++ink_steps < 201)
        {
            ink_motion(277 + ink_steps % 101, 518 + 5 * (ink_steps / 101));
        }
        else
        {
            ink_phase = 6;
            resume_ink_dis();
            puts("PASS 201 local ink motions before release");
            button(SDL_MOUSEBUTTONUP, 382, 523);
            shortcut(SDLK_s);
            memset(&event, 0, sizeof(event));
            event.type = SDL_QUIT;
            post(&event);
        }
    }
}


/* EN: Open the blank SVG before injecting any samples of the closed stroke.
 * RU: Открыть пустой SVG до передачи точек замкнутого мазка. */
static void sweep_burst(void)
{
    sweep_phase = 1;
    open_vector();
}


/* EN: Queue a pointer event at a prescribed elapsed time, including wrap.
 * SDL_PushEvent would replace the timestamp with the current clock reading.
 * RU: Поставить событие указателя с заданным временем, включая переполнение.
 * SDL_PushEvent заменил бы время текущим значением часов. */
static void sweep_pointer(Uint32 type, Uint32 elapsed, int x, int y)
{
    SDL_Event event;
    memset(&event, 0, sizeof(event));
    event.type = type;
    event.common.timestamp = sweep_time + elapsed;
    if (type == SDL_MOUSEMOTION)
    {
        event.motion.state = SDL_BUTTON_LMASK;
        event.motion.x = x;
        event.motion.y = y;
    }
    else
    {
        event.button.button = BUTTON_LEFT;
        event.button.state = type == SDL_MOUSEBUTTONDOWN ? SDL_PRESSED : SDL_RELEASED;
        event.button.x = x;
        event.button.y = y;
    }
    if (SDL_PeepEvents(&event, 1, SDL_ADDEVENT, type, type) != 1)
    {
        fprintf(stderr, "paired sweep event: %s\n", SDL_GetError());
        exit(2);
    }
}


/* EN: Wait for each previewed side, inject jitter, then verify Undo and Redo
 * through presented pixels before saving the restored stroke.
 * RU: Дождаться предпросмотра каждой стороны, добавить подрагивания и проверить
 * Undo и Redo по показанным пикселям перед сохранением восстановленного мазка. */
static void inspect_sweep_frame(SDL_Renderer *renderer)
{
    SDL_Event event;
    if (sweep_phase == 1 && ink_pixel(renderer, 220, 490) == 0xffffff &&
        ink_pixel(renderer, 285, 448) == 0xffffff)
    {
        sweep_phase = 2;
        text("++++++++");
        sweep_time = (Uint32)-20;
        sweep_pointer(SDL_MOUSEBUTTONDOWN, 0, 160, 448);
        sweep_pointer(SDL_MOUSEMOTION, 39, 204, 477);
        sweep_pointer(SDL_MOUSEMOTION, 40, 285, 448);
        sweep_pointer(SDL_MOUSEMOTION, 50, 287, 446);
        sweep_pointer(SDL_MOUSEMOTION, 60, 283, 450);
    }
    else if (sweep_phase == 2 && ink_pixel(renderer, 282, 448) == 0x245ac4)
    {
        sweep_phase = 3;
        sweep_pointer(SDL_MOUSEMOTION, 100, 285, 540);
    }
    else if (sweep_phase == 3 && ink_pixel(renderer, 285, 490) == 0x245ac4)
    {
        sweep_phase = 4;
        sweep_pointer(SDL_MOUSEMOTION, 160, 160, 540);
    }
    else if (sweep_phase == 4 && ink_pixel(renderer, 220, 540) == 0x245ac4)
    {
        sweep_phase = 5;
        sweep_pointer(SDL_MOUSEMOTION, 220, 160, 448);
    }
    else if (sweep_phase == 5 && ink_pixel(renderer, 160, 490) == 0x245ac4)
    {
        sweep_phase = 6;
        sweep_pointer(SDL_MOUSEMOTION, 269, 210, 470);
    }
    else if (sweep_phase == 6 && ink_pixel(renderer, 210, 470) == 0x245ac4)
    {
        sweep_phase = 7;
        sweep_pointer(SDL_MOUSEBUTTONUP, 270, 160, 448);
    }
    else if (sweep_phase == 7 && ink_pixel(renderer, 220, 540) == 0x245ac4 &&
        ink_pixel(renderer, 210, 470) == 0xffffff)
    {
        sweep_phase = 8;
        shortcut(SDLK_z);
    }
    else if (sweep_phase == 8 && ink_pixel(renderer, 220, 540) == 0xffffff)
    {
        sweep_phase = 9;
        shortcut(SDLK_y);
    }
    else if (sweep_phase == 9 && ink_pixel(renderer, 220, 540) == 0x245ac4)
    {
        sweep_phase = 10;
        puts("PASS one Undo and Redo for the smoothed brush stroke");
        shortcut(SDLK_s);
        memset(&event, 0, sizeof(event));
        event.type = SDL_QUIT;
        post(&event);
    }
}


/* EN: Read the actual renderer before present, then forward the host operation.
 * RU: Прочитать настоящий растр перед показом и передать вызов SDL. */
void SDLCALL SDL_RenderPresent(SDL_Renderer *renderer)
{
    static void (SDLCALL *present_real)(SDL_Renderer *); /* Resolved once on the SDL owner. */
    if (!present_real)
    {
        void *symbol = dlsym(RTLD_NEXT, "SDL_RenderPresent");
        memcpy(&present_real, &symbol, sizeof(present_real));
        if (!present_real)
        {
            exit(2);
        }
    }
    if (ink_phase && ink_phase < 6)
    {
        inspect_ink_frame(renderer);
    }
    if (sweep_phase && sweep_phase < 10)
    {
        inspect_sweep_frame(renderer);
    }
    present_real(renderer);
}
