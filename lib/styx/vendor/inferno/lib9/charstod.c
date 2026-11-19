/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 charstod support.
 * RU: исходная POSIX-поддержка lib9 charstod.
 */
#include "lib9.h"

/*
 * Reads a floating-point number by interpreting successive characters
 * returned by (*f)(vp).  The last call it makes to f terminates the
 * scan, so is not a character in the number.  It may therefore be
 * necessary to back up the input stream up one byte after calling charstod.
 */

/* Continue scanning after storage fills so the last callback still ends the token. */
#define ADVANCE                                                                                                        \
    do                                                                                                                 \
    {                                                                                                                  \
        if (s < e)                                                                                                     \
            *s++ = c;                                                                                                  \
        else                                                                                                           \
            overflow = 1;                                                                                              \
        c = (*f)(vp);                                                                                                  \
    } while (0)

/* Bound stored text while consuming the entire number and one terminator. */


/* [standalone-contract:charstod]
 * EN: charstod: original POSIX lib9 charstod support.
 * RU: charstod: исходная POSIX-поддержка lib9 charstod.
 * int (*f)(void *) (int (*f)(void *)): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * vp (void *vp): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: double; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: double; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
double charstod(int (*f)(void *), void *vp)
{
    char str[400], *s, *e, *start;
    int c, overflow;

    overflow = 0;
    s = str;
    e = str + sizeof str - 1;
    c = (*f)(vp);
    while (c == ' ' || c == '\t')
    {
        c = (*f)(vp);
    }
    if (c == '-' || c == '+')
    {
        ADVANCE;
    }
    start = s;
    while (c >= '0' && c <= '9')
    {
        ADVANCE;
    }
    if (c == '.')
    {
        ADVANCE;
        while (c >= '0' && c <= '9')
        {
            ADVANCE;
        }
    }
    if (s > start && (c == 'e' || c == 'E'))
    {
        ADVANCE;
        if (c == '-' || c == '+')
        {
            ADVANCE;
        }
        while (c >= '0' && c <= '9')
        {
            ADVANCE;
        }
    }
    else if (s == start && (c == 'i' || c == 'I'))
    {
        ADVANCE;
        if (c != 'n' && c != 'N')
        {
            return NaN();
        }
        ADVANCE;
        if (c != 'f' && c != 'F')
        {
            return NaN();
        }
        ADVANCE;
        if (c != 'i' && c != 'I')
        {
            return NaN();
        }
        ADVANCE;
        if (c != 'n' && c != 'N')
        {
            return NaN();
        }
        ADVANCE;
        if (c != 'i' && c != 'I')
        {
            return NaN();
        }
        ADVANCE;
        if (c != 't' && c != 'T')
        {
            return NaN();
        }
        ADVANCE;
        if (c != 'y' && c != 'Y')
        {
            return NaN();
        }
        ADVANCE; /* so caller can back up uniformly */
    }
    else if (s == start && (c == 'n' || c == 'N'))
    {
        ADVANCE;
        if (c != 'a' && c != 'A')
        {
            return NaN();
        }
        ADVANCE;
        if (c != 'n' && c != 'N')
        {
            return NaN();
        }
        ADVANCE; /* so caller can back up uniformly */
    }
    if (overflow)
    {
        return NaN();
    }
    *s = 0;
    return strtod(str, &s);
}
