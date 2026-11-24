/* Copyright (c) 2026 bhgv
 * EN: Verify local pixels while the fixture's own Dis child cannot send a scene.
 * RU: Проверить локальные пиксели, пока дочерний Dis теста не может прислать сцену.
 */
#include <signal.h>
#include <unistd.h>

static pid_t ink_dis;        /* Verified child in this fixture's process group. */
static unsigned ink_phase;  /* Zero until requested; 1..6 track presented transitions. */
static unsigned ink_steps;  /* Samples posted after the second stroke becomes visible. */
static unsigned sweep_phase; /* Blank canvas, four previewed sides, then completed save. */


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


/* EN: Wait for each side to appear in the local ink preview before moving
 * to the next corner. Release at the original position and save normally.
 * RU: Дождаться каждого участка локального следа, затем перейти к следующему
 * углу. Отпустить в исходной позиции и сохранить обычным способом. */
static void inspect_sweep_frame(SDL_Renderer *renderer)
{
    SDL_Event event;
    if (sweep_phase == 1 && ink_pixel(renderer, 220, 490) == 0xffffff &&
        ink_pixel(renderer, 285, 448) == 0xffffff)
    {
        sweep_phase = 2;
        text("++++++++");
        button(SDL_MOUSEBUTTONDOWN, 160, 448);
        ink_motion(285, 448);
    }
    else if (sweep_phase == 2 && ink_pixel(renderer, 220, 448) == 0x245ac4)
    {
        sweep_phase = 3;
        ink_motion(285, 540);
    }
    else if (sweep_phase == 3 && ink_pixel(renderer, 285, 490) == 0x245ac4)
    {
        sweep_phase = 4;
        ink_motion(160, 540);
    }
    else if (sweep_phase == 4 && ink_pixel(renderer, 220, 540) == 0x245ac4)
    {
        sweep_phase = 5;
        ink_motion(160, 448);
    }
    else if (sweep_phase == 5 && ink_pixel(renderer, 160, 490) == 0x245ac4)
    {
        sweep_phase = 6;
        button(SDL_MOUSEBUTTONUP, 160, 448);
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
    if (sweep_phase && sweep_phase < 6)
    {
        inspect_sweep_frame(renderer);
    }
    present_real(renderer);
}
