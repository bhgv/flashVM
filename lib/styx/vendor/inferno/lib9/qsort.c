/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 qsort support.
 * RU: исходная POSIX-поддержка lib9 qsort.
 */
/*
 * qsort -- simple quicksort
 */

#include "lib9.h"

typedef struct
{
    int (*cmp)(void *, void *);
    void (*swap)(char *, char *, long);
    long es;
} Sort;

/* Exchange an element whose address or size needs bytewise access. */


/* [standalone-contract:swapb]
 * EN: swapb: original POSIX lib9 qsort support.
 * RU: swapb: исходная POSIX-поддержка lib9 qsort.
 * i (char *i): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * j (char *j): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * es (long es): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void swapb(char *i, char *j, long es)
{
    char c;

    do
    {
        c = *i;
        *i++ = *j;
        *j++ = c;
        es--;
    } while (es != 0);
}

/* Exchange aligned native-word spans selected by the public entry point. */


/* [standalone-contract:swapi]
 * EN: swapi: original POSIX lib9 qsort support.
 * RU: swapi: исходная POSIX-поддержка lib9 qsort.
 * ii (char *ii): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * ij (char *ij): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * es (long es): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void swapi(char *ii, char *ij, long es)
{
    long *i, *j, c;

    i = (long *)ii;
    j = (long *)ij;
    do
    {
        c = *i;
        *i++ = *j;
        *j++ = c;
        es -= sizeof(long);
    } while (es != 0);
}

/* Choose the median of three spread-out elements without moving storage. */


/* [standalone-contract:pivot]
 * EN: pivot: original POSIX lib9 qsort support.
 * RU: pivot: исходная POSIX-поддержка lib9 qsort.
 * a (char *a): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * n (long n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * p (Sort *p): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static char *pivot(char *a, long n, Sort *p)
{
    long j;
    char *pi, *pj, *pk;

    j = n / 6 * p->es;
    pi = a + j;  /* 1/6 */
    j += j;
    pj = pi + j; /* 1/2 */
    pk = pj + j; /* 5/6 */
    if (p->cmp(pi, pj) < 0)
    {
        if (p->cmp(pi, pk) < 0)
        {
            if (p->cmp(pj, pk) < 0)
            {
                return pj;
            }
            return pk;
        }
        return pi;
    }
    if (p->cmp(pj, pk) < 0)
    {
        if (p->cmp(pi, pk) < 0)
        {
            return pi;
        }
        return pk;
    }
    return pj;
}

/* Recurse on the smaller partition and iterate over the larger one. */


/* [standalone-contract:qsorts]
 * EN: qsorts: original POSIX lib9 qsort support.
 * RU: qsorts: исходная POSIX-поддержка lib9 qsort.
 * a (char *a): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * n (long n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * p (Sort *p): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void qsorts(char *a, long n, Sort *p)
{
    long j, es;
    char *pi, *pj, *pn;

    es = p->es;
    while (n > 1)
    {
        if (n > 10)
        {
            pi = pivot(a, n, p);
        }
        else
        {
            pi = a + (n >> 1) * es;
        }

        p->swap(a, pi, es);
        pi = a;
        pn = a + n * es;
        pj = pn;
        for (;;)
        {
            do
            {
                pi += es;
            } while (pi < pn && p->cmp(pi, a) < 0);
            do
            {
                pj -= es;
            } while (pj > a && p->cmp(pj, a) > 0);
            if (pj < pi)
            {
                break;
            }
            p->swap(pi, pj, es);
        }
        p->swap(a, pj, es);
        j = (pj - a) / es;

        n = n - j - 1;
        if (j < n)
        {
            qsorts(a, j, p);
            a += (j + 1) * es;
        }
        else
        {
            qsorts(a + (j + 1) * es, n, p);
            n = j;
        }
    }
}

/* Ignore empty/invalid spans before partitioning or multiplying native counts. */


/* [standalone-contract:qsort]
 * EN: qsort: original POSIX lib9 qsort support.
 * RU: qsort: исходная POSIX-поддержка lib9 qsort.
 * va (void *va): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * n (long n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * es (long es): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * int (*cmp)(void *, void *) (int (*cmp)(void *, void *)): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void qsort(void *va, long n, long es, int (*cmp)(void *, void *))
{
    Sort s;

    if (n < 2 || es <= 0 || n > (long)(~(ulong)0 >> 1) / es)
    {
        return;
    }
    s.cmp = cmp;
    s.es = es;
    s.swap = swapi;
    if (((uintptr)va | es) % sizeof(long))
    {
        s.swap = swapb;
    }
    qsorts((char *)va, n, &s);
}
