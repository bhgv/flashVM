/* Copyright (c) 2026 bhgv
 * EN: Native formatter registry lock; callbacks execute after releasing it.
 * RU: Native formatter registry lock; callbacks выполняются после unlock.
 */
#include <pthread.h>
#include <stdlib.h>

static pthread_mutex_t fmtmutex = PTHREAD_MUTEX_INITIALIZER; /* Short native registry transactions. */

/* EN/RU: Lock formatter metadata; no VM execution or callbacks.
 * Захватить formatter metadata; без VM execution/callbacks. */


/* [standalone-contract:_fmtlock]
 * EN: _fmtlock: posix/fmtlock implementation and native state.
 * RU: _fmtlock: реализация posix/fmtlock и нативное состояние.
 * Parameters: none. Параметры: нет.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void _fmtlock(void)
{
    if (pthread_mutex_lock(&fmtmutex) != 0)
    {
        abort();
    }
}

/* EN/RU: Release metadata before invoking the selected formatter.
 * Освободить metadata до вызова formatter. */


/* [standalone-contract:_fmtunlock]
 * EN: _fmtunlock: posix/fmtlock implementation and native state.
 * RU: _fmtunlock: реализация posix/fmtlock и нативное состояние.
 * Parameters: none. Параметры: нет.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void _fmtunlock(void)
{
    if (pthread_mutex_unlock(&fmtmutex) != 0)
    {
        abort();
    }
}
