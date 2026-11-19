/* Copyright (c) 2026 bhgv
 * EN: Fault-inject the real teaching program's partial initialization; require no
 * engine or initialized barrier remains after failed allocation/thread creation.
 * RU: Внедрять ошибки в частичную инициализацию реального примера; после отказа
 * allocation/thread creation не должно остаться engine или initialized barrier.
 */
#define _POSIX_C_SOURCE 200809L
#include "order.h"
#include <pthread.h>
#include <errno.h>
#include <stdio.h>

static int stage;          /* 1/2: fail engine constructor; 3: fail barrier init; 4: fail thread creation. */
static int constructors;   /* Identifies the first or second engine constructor in this run. */
static int engines;        /* Successfully created engines not yet freed by the teaching program. */
static int barriers;       /* Successfully initialized barriers not yet destroyed. */
static int unexpected;     /* Counts an operation that should be unreachable for this injected failure. */

extern int queue_example_main(void); /* The same queues.c, compiled with its entry point renamed. */
extern StyxOrder *__real_styxordercreate(unsigned, unsigned long long);
extern void __real_styxorderfree(StyxOrder *);
extern int __real_pthread_barrier_init(pthread_barrier_t *, const pthread_barrierattr_t *, unsigned);
extern int __real_pthread_barrier_destroy(pthread_barrier_t *);


/* EN: Fail the selected engine allocation, otherwise call the real constructor.
 * limit/window are forwarded unchanged. Returns owned engine/NULL; track only
 * successful ownership transfers. Used serially by the main-thread test.
 * RU: Отказать в выбранном allocation, иначе вызвать настоящий constructor.
 * limit/window передаются без изменений. Вернуть owned engine/NULL; считать
 * только успешную передачу владения. Последовательные вызовы тестового main.
 */
StyxOrder *__wrap_styxordercreate(unsigned limit, unsigned long long window)
{
    constructors++;
    if (constructors == stage && stage <= 2)
    {
        return NULL;
    }
    StyxOrder *engine = __real_styxordercreate(limit, window);
    engines += engine != NULL;
    return engine;
}


/* EN: Consume engine exactly as the real release does; NULL transfers nothing.
 * Track live allocations so process exit cannot hide missing cleanup.
 * RU: Поглотить engine настоящим release; NULL ничего не передаёт.
 * Считать живые allocations, чтобы process exit не скрывал пропущенный cleanup.
 */
void __wrap_styxorderfree(StyxOrder *engine)
{
    engines -= engine != NULL;
    __real_styxorderfree(engine);
}


/* EN: barrier/attributes/count are forwarded to the real constructor except in
 * stage 3, which returns EAGAIN without initializing anything. Track success.
 * RU: barrier/attributes/count передаются настоящему constructor, кроме этапа 3:
 * вернуть EAGAIN без инициализации. Считать только успешную инициализацию.
 */
int __wrap_pthread_barrier_init(pthread_barrier_t *barrier, const pthread_barrierattr_t *attributes, unsigned count)
{
    if (stage == 3)
    {
        return EAGAIN;
    }
    int error = __real_pthread_barrier_init(barrier, attributes, count);
    barriers += error == 0;
    return error;
}


/* EN: Destroy borrowed barrier using the real API; decrement only on success.
 * No worker is created by these failure cases, so there can be no barrier users.
 * RU: Уничтожить barrier настоящим API; уменьшить счётчик только при успехе.
 * Worker в этих случаях не создаётся, использующих barrier потоков нет.
 */
int __wrap_pthread_barrier_destroy(pthread_barrier_t *barrier)
{
    int error = __real_pthread_barrier_destroy(barrier);
    barriers -= error == 0;
    return error;
}


/* EN: Inject EAGAIN without starting a worker; thread/attributes/function/argument
 * are borrowed and deliberately unused. Creation must be attempted only in stage 4.
 * RU: Выдать EAGAIN без создания worker; thread/attributes/function/argument
 * заимствованы и не используются. Попытка создания допустима только на этапе 4.
 */
int __wrap_pthread_create(pthread_t *thread, const pthread_attr_t *attributes, void *(*function)(void *), void *argument)
{
    (void)thread;
    (void)attributes;
    (void)function;
    (void)argument;
    unexpected += stage != 4;
    return EAGAIN;
}


/* EN: No arguments. Run four real failure paths; return 1 on leaked ownership,
 * unexpected progress or a false success, otherwise 0. Test globals are serial.
 * RU: Аргументов нет. Пройти четыре реальные ошибки; вернуть 1 при утечке владения,
 * неожиданном продолжении или ложном успехе, иначе 0. Globals последовательны.
 */
int main(void)
{
    for (stage = 1; stage <= 4; stage++)
    {
        constructors = 0;
        if (queue_example_main() != 1 || engines || barriers || unexpected)
        {
            fprintf(stderr, "stage %d: engines=%d barriers=%d unexpected=%d\n", stage, engines, barriers, unexpected);
            return 1;
        }
    }
    puts("PASS queue example: both allocation failures, barrier failure and thread creation cleanup");
    return 0;
}
