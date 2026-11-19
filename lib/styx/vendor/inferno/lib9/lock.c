/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 lock support.
 * RU: исходная POSIX-поддержка lib9 lock.
 */
#include "dat.h"
#include "fns.h"
#include "error.h"

/* [standalone-contract:lock]
 * EN: lock: original POSIX lib9 lock support.
 * RU: lock: исходная POSIX-поддержка lib9 lock.
 * l (Lock *l): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void lock(Lock *l)
{
    int i;

    if (_tas(&l->val) == 0)
    {
        return;
    }
    for (i = 0; i < 100; i++)
    {
        if (_tas(&l->val) == 0)
        {
            return;
        }
        osyield();
    }
    for (i = 1;; i++)
    {
        if (_tas(&l->val) == 0)
        {
            return;
        }
        osmillisleep(i * 10);
        if (i > 100)
        {
            osyield();
            i = 1;
        }
    }
}

/* [standalone-contract:canlock]
 * EN: canlock: original POSIX lib9 lock support.
 * RU: canlock: исходная POSIX-поддержка lib9 lock.
 * l (Lock *l): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int canlock(Lock *l)
{
    return _tas(&l->val) == 0;
}

/* [standalone-contract:unlock]
 * EN: unlock: original POSIX lib9 lock support.
 * RU: unlock: исходная POSIX-поддержка lib9 lock.
 * l (Lock *l): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void unlock(Lock *l)
{
    l->val = 0;
}

/* [standalone-contract:qlock]
 * EN: qlock: original POSIX lib9 lock support.
 * RU: qlock: исходная POSIX-поддержка lib9 lock.
 * q (QLock *q): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void qlock(QLock *q)
{
    Proc *p;

    lock(&q->use);
    if (!q->locked)
    {
        q->locked = 1;
        unlock(&q->use);
        return;
    }
    p = q->tail;
    if (p == 0)
    {
        q->head = up;
    }
    else
    {
        p->qnext = up;
    }
    q->tail = up;
    up->qnext = 0;
    unlock(&q->use);
    osblock();
}

/* [standalone-contract:canqlock]
 * EN: canqlock: original POSIX lib9 lock support.
 * RU: canqlock: исходная POSIX-поддержка lib9 lock.
 * q (QLock *q): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int canqlock(QLock *q)
{
    if (!canlock(&q->use))
    {
        return 0;
    }
    if (q->locked)
    {
        unlock(&q->use);
        return 0;
    }
    q->locked = 1;
    unlock(&q->use);
    return 1;
}

/* [standalone-contract:qunlock]
 * EN: qunlock: original POSIX lib9 lock support.
 * RU: qunlock: исходная POSIX-поддержка lib9 lock.
 * q (QLock *q): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void qunlock(QLock *q)
{
    Proc *p;

    lock(&q->use);
    p = q->head;
    if (p)
    {
        q->head = p->qnext;
        if (q->head == 0)
        {
            q->tail = 0;
        }
        unlock(&q->use);
        osready(p);
        return;
    }
    q->locked = 0;
    unlock(&q->use);
}

/* [standalone-contract:rlock]
 * EN: rlock: original POSIX lib9 lock support.
 * RU: rlock: исходная POSIX-поддержка lib9 lock.
 * l (RWlock *l): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void rlock(RWlock *l)
{
    qlock(&l->x); /* wait here for writers and exclusion */
    lock(&l->l);
    l->readers++;
    canqlock(&l->k); /* block writers if we are the first reader */
    unlock(&l->l);
    qunlock(&l->x);
}

/* same as rlock but punts if there are any writers waiting */


/* [standalone-contract:canrlock]
 * EN: canrlock: original POSIX lib9 lock support.
 * RU: canrlock: исходная POSIX-поддержка lib9 lock.
 * l (RWlock *l): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int canrlock(RWlock *l)
{
    if (!canqlock(&l->x))
    {
        return 0;
    }
    lock(&l->l);
    l->readers++;
    canqlock(&l->k); /* block writers if we are the first reader */
    unlock(&l->l);
    qunlock(&l->x);
    return 1;
}

/* [standalone-contract:runlock]
 * EN: runlock: original POSIX lib9 lock support.
 * RU: runlock: исходная POSIX-поддержка lib9 lock.
 * l (RWlock *l): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void runlock(RWlock *l)
{
    lock(&l->l);
    if (--l->readers == 0) /* last reader out allows writers */
    {
        qunlock(&l->k);
    }
    unlock(&l->l);
}

/* [standalone-contract:wlock]
 * EN: wlock: original POSIX lib9 lock support.
 * RU: wlock: исходная POSIX-поддержка lib9 lock.
 * l (RWlock *l): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void wlock(RWlock *l)
{
    qlock(&l->x); /* wait here for writers and exclusion */
    qlock(&l->k); /* wait here for last reader */
}

/* [standalone-contract:wunlock]
 * EN: wunlock: original POSIX lib9 lock support.
 * RU: wunlock: исходная POSIX-поддержка lib9 lock.
 * l (RWlock *l): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void wunlock(RWlock *l)
{
    qunlock(&l->k);
    qunlock(&l->x);
}
