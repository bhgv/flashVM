/* Copyright (c) 2026 bhgv
 * EN: Owner-only UI deadlines and SDL blocking wait; no redraw is requested here.
 * RU: Дедлайны UI и блокирующее ожидание SDL; перерисовка здесь не запрашивается.
 */
#ifdef DVM_EDITOR_SDL
static int software_event_wait; /* Dummy/legacy SDL has no native event wait; use a bounded condition fallback. */
static Uint32 editor_wake_event; /* Immutable registered SDL type, published before session.running. */
#endif


/* EN: Clear wake registration on UI main before creation or after all senders
 * have joined. No SDK call; safe for partial/headless creation as well.
 * RU: Сбросить регистрацию в UI main до создания или после join отправителей.
 * Без вызовов SDK; допустимо для неполного/headless создания. */
void dvm_editor_idle_reset(void)
{
#ifdef DVM_EDITOR_SDL
    editor_wake_event = 0;
    software_event_wait = 0;
#endif
}


/* EN: Initialize the private SDL wake record on UI main after SDL_Init. Return
 * one on success, zero on exhaustion; headless/non-SDL sessions need no event.
 * RU: После SDL_Init зарегистрировать событие пробуждения в UI main. Вернуть
 * один при успехе, ноль при исчерпании; headless/без SDL событие не требуется. */
int dvm_editor_idle_initialize(void)
{
#ifdef DVM_EDITOR_SDL
    if (!events || window_replay)
    {
        const char *driver = SDL_GetCurrentVideoDriver();
        SDL_version version;
        SDL_GetVersion(&version);
        software_event_wait = (driver && (!strcmp(driver, "dummy") || !strcmp(driver, "offscreen"))) ||
            SDL_VERSIONNUM(version.major, version.minor, version.patch) < SDL_VERSIONNUM(2, 0, 18);
        editor_wake_event = SDL_RegisterEvents(1);
        return editor_wake_event != (Uint32)-1;
    }
#endif
    return 1;
}


/* EN: Queue one native wake from a live session sender. The caller pins SDL
 * lifetime with the session mutex; no FlashVM/Dis heap or window is accessed.
 * RU: Поставить native wake от отправителя живой сессии. Вызывающий удерживает
 * жизнь SDL замком сессии; куча FlashVM/Dis и окно не затрагиваются. */
void dvm_editor_idle_notify(void)
{
#ifdef DVM_EDITOR_SDL
    if (editor_wake_event && editor_wake_event != (Uint32)-1)
    {
        SDL_Event wake;
        memset(&wake, 0, sizeof(wake));
        wake.type = editor_wake_event;
        SDL_PushEvent(&wake);
    }
#endif
}


/* EN: Query the next useful wake delay on UI main, in milliseconds. One second
 * is a recovery ceiling for failed native wake delivery, not a frame clock.
 * Timers, unfinished raster, batching and actual animation shorten it. No
 * callbacks, painting or state mutation occurs; a timer-query failure returns zero.
 * RU: Получить задержку полезного пробуждения в UI main, мс. Секунда — предел
 * восстановления при отказе native wake, а не кадровые часы. Таймер, raster,
 * batching и настоящая анимация сокращают срок. Нет callbacks, рисования или
 * мутации; при ошибке запроса таймера вернуть ноль. */
unsigned long dvm_editor_idle_delay(void)
{
    unsigned long delay = 1000;
    int pending;
    if (!context || fvm_timers_next_delay(context, delay, &delay, &pending) != FVM_OK)
    {
        return 0;
    }
    if (raster_pending && delay > 8)
    {
        delay = 8;
    }
#ifdef DVM_EDITOR_SDL
    /* EN/RU: SDL's dummy/old fallback itself loops at 1 ms inside WaitEventTimeout.
     * Dummy/старый SDL сам опрашивает через 1 мс внутри WaitEventTimeout. */
    if (software_event_wait && delay > 16)
    {
        delay = 16;
    }
    int input_ready = complete || (postponed_valid && dvm_desktop_local_ready() &&
        (postponed_event.type == SDL_MOUSEMOTION || postponed_event.type == SDL_FINGERMOTION));
    if (postponed_valid && input_ready && !presentation_blocks_input())
    {
        return 0;
    }
    if (input_ready && !events && dvm_desktop_animating() && delay > 8)
    {
        delay = 8;
    }
    if (input_ready && desktop_dirty && paint_due && !raster_pending)
    {
        Uint32 age = SDL_GetTicks() - desktop_dirty_since;
        unsigned long batch = ink_token ? EDITOR_INK_BATCH_MS : EDITOR_KEYBOARD_BATCH_MS;
        unsigned long remaining = age < batch ? batch - age : 0;
        if (remaining < delay)
        {
            delay = remaining;
        }
    }
#endif
    return delay;
}


/* EN: Wait on SDL events up to delay ms, preserving a received ordinary event
 * in the existing FIFO slot. Return one when SDL is used, zero for replay or
 * non-SDL backends (the launcher uses its condition there). Only UI main calls.
 * No frame is invalidated; inactive animation time cannot become a future jump.
 * RU: Ждать SDL-событие до delay мс, сохранив обычное событие в FIFO-слоте.
 * Вернуть один для SDL, ноль для replay/без SDL (там launcher ждёт condition).
 * Только UI main. Кадр не инвалидируется; простой не даёт скачка новой анимации. */
int dvm_editor_idle_wait(unsigned long delay)
{
#ifdef DVM_EDITOR_SDL
    if (!events || window_replay)
    {
        SDL_Event event;
        if (postponed_valid || software_event_wait)
        {
            return 0;
        }
        if (SDL_WaitEventTimeout(&event, (int)delay))
        {
            if (event.type != editor_wake_event)
            {
                postponed_event = event;
                postponed_valid = 1;
            }
        }
        if (!dvm_desktop_animating())
        {
            desktop_animation_clock = SDL_GetTicks();
            desktop_animation_clock_set = 1;
        }
        return 1;
    }
#else
    (void)delay;
#endif
    return 0;
}
