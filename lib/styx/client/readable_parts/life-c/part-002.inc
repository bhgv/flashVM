/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of life.c; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */
/* [standalone-contract:styxlifeobserve]
 * EN: styxlifeobserve: native client life operations and lifetime.
 * RU: styxlifeobserve: нативные client-операции life и время жизни.
 * life (StyxLife *life): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * receive (StyxLinkReceive receive): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * state (void *state): Opaque callback context; its lifetime/transfer is specified by the creating API.
 * RU: Непрозрачный контекст callback; время жизни/передача заданы создающим API.
 * void (*release)(void *) (void (*release)(void *)): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: StyxWatch *; direct result constants in this body: NULL. Forwarded results preserve the called API's status.
 * Возврат: StyxWatch *; прямые константы результата тела: NULL. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
StyxWatch *styxlifeobserve(StyxLife *life, StyxLinkReceive receive, void *state, void (*release)(void *))
{
    if (!life || !receive)
    {
        return NULL;
    }
    StyxWatch *watch = calloc(1, sizeof(*watch));
    if (!watch)
    {
        return NULL;
    }
    pthread_mutex_lock(&life->lock);
    if (life->quiet || life->watchers == 256 || life->references == (size_t)-1)
    {
        pthread_mutex_unlock(&life->lock);
        free(watch);
        return NULL;
    }
    watch->life = life;
    watch->observe = receive;
    watch->state = state;
    watch->release = release;
    watch->active = 1;
    watch->references = 3;
    watch->next = life->observers;
    life->observers = watch;
    life->references++;
    life->watchers++;
    StyxLinkEvent event = life->event;
    pthread_mutex_unlock(&life->lock);
    receive(state, &event);
    drop(watch);
    return watch;
}

/* EN/RU: See life.h; local copy only, no remote capability or transport access.
 * См. life.h; только local copy без transport access. */


/* [standalone-contract:styxlifesnapshot]
 * EN: styxlifesnapshot: native client life operations and lifetime.
 * RU: styxlifesnapshot: нативные client-операции life и время жизни.
 * life (StyxLife *life): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * output (StyxLinkEvent *output): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int styxlifesnapshot(StyxLife *life, StyxLinkEvent *output)
{
    if (!life || !output)
    {
        return 0;
    }
    pthread_mutex_lock(&life->lock);
    *output = life->event;
    pthread_mutex_unlock(&life->lock);
    return 1;
}
