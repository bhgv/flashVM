/* Copyright (c) 2026 bhgv
 * EN: Independent SDL input producer and unchanged-frame/CPU observations.
 * RU: Независимый отправитель SDL-событий и проверка CPU/неизменности кадров.
 */
#define _POSIX_C_SOURCE 200809L
#include <SDL.h>
#include "flashvm/render_queue.h"
#include "flashvm/timers.h"
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static FVM_Context *owner_context; /* Borrowed only on UI main, until SDL_Quit. */
static double timer_started; /* Owner-only deadline observation in seconds. */
static pthread_t producer; /* Joined before SDL_Quit on the UI owner. */
static int started; /* Only UI main changes this publication flag. */
static Uint32 controls; /* Test-only immutable SDL event type. */
static Uint32 window_id; /* Borrowed window identity copied before producer startup. */
static unsigned long presents; /* Atomic display-present observation, never a VM pointer. */
static unsigned long polls; /* Atomic UI-loop observations, sampled only while quiet. */
static unsigned long frames; /* Atomic immutable-raster submission observation. */

FVM_Result __real_fvm_timers_poll(FVM_Context *context, unsigned int *delivered);
int __real_dvm_editor_ui_poll(int stopping);
int __real_SDL_PollEvent(SDL_Event *event);
int __real_SDL_WaitEventTimeout(SDL_Event *event, int timeout);
void __real_SDL_RenderPresent(SDL_Renderer *renderer);
void __real_SDL_Quit(void);
FVM_Result __real_fvm_render_queue_submit(FVM_RenderQueue *queue, void *job);


/* EN/RU: Read clock id in seconds; negative signals native clock failure.
 * Прочитать часы id в секундах; отрицательное значение означает ошибку. */
static double seconds(clockid_t id)
{
    struct timespec value;
    if (clock_gettime(id, &value))
    {
        return -1;
    }
    return (double)value.tv_sec + (double)value.tv_nsec / 1000000000.0;
}


/* EN/RU: Observe a timer on its context owner; borrowed arguments are unused.
 * Учесть таймер во владельце контекста; заимствованные аргументы не используются. */
static FVM_Result timer_ready(FVM_Context *context, FVM_Object *receiver, unsigned count,
    const FVM_Value *arguments, FVM_Value *result, void *user)
{
    (void)context;
    (void)receiver;
    (void)count;
    (void)arguments;
    (void)user;
    fvm_value_undefined(result);
    fprintf(stderr, "SDLIDLE timer_ms=%.2f\n", (seconds(CLOCK_MONOTONIC) - timer_started) * 1000);
    return FVM_OK;
}


/* EN/RU: Schedule a real 75 ms callback on UI main, without invalidating a frame.
 * Install scalar builtins for single-view contexts that do not use them.
 * Назначить настоящий callback через 75 мс в UI main, без инвалидирования кадра.
 * Установить scalar builtins для single-view контекста, где они не используются. */
static void schedule_timer(void)
{
    FVM_Object *callback;
    FVM_Value arguments[2];
    FVM_Value result;
    if (!owner_context || fvm_standard_install(owner_context) != FVM_OK ||
        fvm_function_create_native(owner_context, timer_ready, NULL, NULL, &callback) != FVM_OK)
    {
        return;
    }
    fvm_gc_pin(owner_context, callback);
    fvm_value_undefined(&arguments[0]);
    arguments[0].type = FVM_VALUE_OBJECT;
    arguments[0].object = callback;
    fvm_value_number(&arguments[1], 75);
    timer_started = seconds(CLOCK_MONOTONIC);
    if (fvm_call(owner_context, "setTimeout", 2, arguments, &result) != FVM_OK)
    {
        fprintf(stderr, "SDLIDLE timer scheduling failed\n");
    }
    fvm_gc_unpin(owner_context, callback);
}


/* EN/RU: Wait a test-producer interval ms without suspending UI or guest owners.
 * Подождать ms в тестовом отправителе; UI и гости не приостанавливаются. */
static void interval(long ms)
{
    struct timespec delay = {ms / 1000, (ms % 1000) * 1000000L};
    while (nanosleep(&delay, &delay))
    {
        /* Restart interrupted test-only sleep with its remaining duration. */
    }
}


/* EN/RU: Queue copied test control code; only UI main interprets its payload.
 * Поставить копию control code; payload обрабатывает только UI main. */
static void control(int code)
{
    SDL_Event event;
    memset(&event, 0, sizeof(event));
    event.type = controls;
    event.user.code = code;
    SDL_PushEvent(&event);
}


/* EN: Measure two quiet seconds named label. Read native counters only; no
 * window/VM access. Counter deltas must be zero when the scene is unchanged.
 * RU: Измерить две тихие секунды label. Читать лишь native-счётчики; без доступа
 * к окну/VM. Если сцена не менялась, приросты счётчиков должны быть нулевыми. */
static void measured(const char *label)
{
    double start = seconds(CLOCK_MONOTONIC);
    double cpu = seconds(CLOCK_PROCESS_CPUTIME_ID);
    unsigned long previous_presents = __atomic_load_n(&presents, __ATOMIC_RELAXED);
    unsigned long previous_polls = __atomic_load_n(&polls, __ATOMIC_RELAXED);
    unsigned long previous_frames = __atomic_load_n(&frames, __ATOMIC_RELAXED);
    interval(2000);
    fprintf(stderr, "SDLIDLE %s cpu=%.2f presents=%lu frames=%lu polls=%lu\n", label,
            100 * (seconds(CLOCK_PROCESS_CPUTIME_ID) - cpu) / (seconds(CLOCK_MONOTONIC) - start),
            __atomic_load_n(&presents, __ATOMIC_RELAXED) - previous_presents,
            __atomic_load_n(&frames, __ATOMIC_RELAXED) - previous_frames,
            __atomic_load_n(&polls, __ATOMIC_RELAXED) - previous_polls);
}


/* EN: Own input scheduling independently of event polling. argument is unused;
 * send only copied SDL records and return before the UI joins/destroys SDL.
 * RU: Планировать ввод независимо от опроса UI. argument не используется;
 * передавать только копии SDL-записей и вернуться до join/удаления SDL. */
static void *inject(void *argument)
{
    SDL_Event text;
    (void)argument;
    interval(1000);
    control(6);
    interval(300);
    measured("visible");
    control(1);
    interval(300);
    measured("minimized");
    control(2);
    interval(500);
    memset(&text, 0, sizeof(text));
    text.type = SDL_TEXTINPUT;
    text.text.windowID = window_id;
    strcpy(text.text.text, "native-");
    SDL_PushEvent(&text);
    interval(500);
    control(3);
    interval(500);
    control(4);
    interval(500);
    control(5);
    return NULL;
}


/* EN/RU: Queue key type/symbol/modifiers on UI main for the borrowed window.
 * Поставить key type/symbol/modifiers в UI main для заимствованного окна. */
static void key(Uint32 type, SDL_Keycode symbol, Uint16 modifiers)
{
    SDL_Event event;
    memset(&event, 0, sizeof(event));
    event.type = type;
    event.key.windowID = window_id;
    event.key.state = type == SDL_KEYDOWN ? SDL_PRESSED : SDL_RELEASED;
    event.key.keysym.sym = symbol;
    event.key.keysym.mod = modifiers;
    SDL_PushEvent(&event);
}


/* EN: Consume only private test control on UI main; keep ordinary SDL FIFO
 * records unchanged. Return zero for consumed control, one for a real event.
 * RU: Обработать только частный control в UI main; обычные SDL-записи сохранить.
 * Вернуть ноль для поглощённого control, один для настоящего события. */
static int filtered(SDL_Event *event)
{
    SDL_Window *window;
    if (event->type != controls)
    {
        return 1;
    }
    window = SDL_GetWindowFromID(window_id);
    if (event->user.code == 6)
    {
        schedule_timer();
    }
    else if (event->user.code == 1)
    {
        SDL_MinimizeWindow(window);
    }
    else if (event->user.code == 2)
    {
        SDL_RestoreWindow(window);
        SDL_SetWindowSize(window, 1000, 700);
    }
    else if (event->user.code == 3)
    {
        SDL_SetModState(KMOD_CTRL);
        key(SDL_KEYDOWN, SDLK_LCTRL, KMOD_CTRL);
        key(SDL_KEYDOWN, SDLK_s, KMOD_CTRL);
        key(SDL_KEYUP, SDLK_s, KMOD_CTRL);
        key(SDL_KEYUP, SDLK_LCTRL, KMOD_NONE);
    }
    else if (event->user.code == 4)
    {
        SDL_SetModState(KMOD_NONE);
    }
    else if (event->user.code == 5)
    {
        SDL_Event quit;
        memset(&quit, 0, sizeof(quit));
        quit.type = SDL_QUIT;
        SDL_SetModState(KMOD_SHIFT);
        SDL_PushEvent(&quit);
    }
    return 0;
}


/* EN/RU: Start the producer on UI main's first poll or blocking wait; one startup.
 * Запустить отправителя при первом poll либо wait UI main, только один раз. */
static int start_producer(void)
{
    if (!started)
    {
        controls = SDL_RegisterEvents(1);
        window_id = SDL_GetWindowID(SDL_GetWindowFromID(1));
        if (controls == (Uint32)-1 || !window_id || pthread_create(&producer, NULL, inject, NULL))
        {
            return 0;
        }
        started = 1;
    }
    return 1;
}


/* EN/RU: First real UI poll starts the private producer; retain real SDL behavior.
 * Первый UI poll запускает отправителя; настоящее поведение SDL сохраняется. */
int __wrap_SDL_PollEvent(SDL_Event *event)
{
    int result;
    if (!start_producer())
    {
        return 0;
    }
    result = __real_SDL_PollEvent(event);
    return result && event ? filtered(event) : result;
}


/* EN/RU: Preserve blocking timeout and ordinary events; consume only test control.
 * Сохранить ожидание/таймаут и обычные события; поглощать только test control. */
int __wrap_SDL_WaitEventTimeout(SDL_Event *event, int timeout)
{
    int result;
    if (!start_producer())
    {
        return 0;
    }
    result = __real_SDL_WaitEventTimeout(event, timeout);
    return result && event ? filtered(event) : result;
}


/* EN/RU: Observe actual presents, then call original renderer with borrowed pointer.
 * Учесть настоящий present и вызвать исходный renderer с заимствованным указателем. */
void __wrap_SDL_RenderPresent(SDL_Renderer *renderer)
{
    __atomic_fetch_add(&presents, 1, __ATOMIC_RELAXED);
    __real_SDL_RenderPresent(renderer);
}


/* EN/RU: Count actual frame submissions; preserve original ownership/result.
 * Учесть отправку кадра; сохранить исходное владение и результат. */
FVM_Result __wrap_fvm_render_queue_submit(FVM_RenderQueue *queue, void *job)
{
    __atomic_fetch_add(&frames, 1, __ATOMIC_RELAXED);
    return __real_fvm_render_queue_submit(queue, job);
}


/* EN/RU: Join the test producer before original SDL destruction on UI main.
 * Присоединить отправителя до настоящего удаления SDL в UI main. */
void __wrap_SDL_Quit(void)
{
    if (started)
    {
        pthread_join(producer, NULL);
        started = 0;
    }
    __real_SDL_Quit();
}


/* EN/RU: Count UI iterations without altering production polling or its result.
 * Учесть итерации UI, не меняя production-опрос и его результат. */
int __wrap_dvm_editor_ui_poll(int stopping)
{
    __atomic_fetch_add(&polls, 1, __ATOMIC_RELAXED);
    return __real_dvm_editor_ui_poll(stopping);
}


/* EN/RU: Borrow the live context on UI main; retain real callback delivery.
 * Заимствовать живой контекст в UI main; сохранить настоящую доставку callbacks. */
FVM_Result __wrap_fvm_timers_poll(FVM_Context *context, unsigned int *delivered)
{
    owner_context = context;
    return __real_fvm_timers_poll(context, delivered);
}


/* EN: Exercise the native-wait branch with dummy SDL when the test flag is set.
 * No production driver is replaced; SDL still owns its real dummy event queue.
 * RU: При тестовом флаге пройти ветку native wait с dummy SDL. Production-драйвер
 * не подменяется; настоящая очередь SDL остаётся у dummy backend. */
const char *__wrap_SDL_GetCurrentVideoDriver(void)
{
    extern const char *__real_SDL_GetCurrentVideoDriver(void);
    return getenv("DVM_TEST_NATIVE_WAIT") ? "test-native-wait" : __real_SDL_GetCurrentVideoDriver();
}
