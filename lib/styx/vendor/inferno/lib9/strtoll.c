/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 strtoll support.
 * RU: исходная POSIX-поддержка lib9 strtoll.
 */
#include "lib9.h"

/* Accumulate signed conversion bit patterns without host signed overflow. */


/* [standalone-contract:strtoll]
 * EN: strtoll: original POSIX lib9 strtoll support.
 * RU: strtoll: исходная POSIX-поддержка lib9 strtoll.
 * nptr (const char *nptr): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * endptr (char **endptr): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * base (int base): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: vlong; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: vlong; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
vlong strtoll(const char *nptr, char **endptr, int base)
{
    const char *p;
    uvlong n;
    int c, v, neg, ndig;

    p = nptr;
    neg = 0;
    n = 0;
    ndig = 0;

    /*
	 * White space
	 */
    for (;; p++)
    {
        switch (*p)
        {
        case ' ':
        case '\t':
        case '\n':
        case '\f':
        case '\r':
        case '\v':
            continue;
        }
        break;
    }

    /*
	 * Sign
	 */
    if (*p == '-' || *p == '+')
    {
        if (*p++ == '-')
        {
            neg = 1;
        }
    }

    /*
	 * Base
	 */
    if (base == 0)
    {
        if (*p != '0')
        {
            base = 10;
        }
        else
        {
            base = 8;
            if (p[1] == 'x' || p[1] == 'X')
            {
                p += 2;
                base = 16;
            }
        }
    }
    else if (base == 16 && *p == '0')
    {
        if (p[1] == 'x' || p[1] == 'X')
        {
            p += 2;
        }
    }
    else if (base < 2 || 36 < base)
    {
        goto Return;
    }

    /*
	 * Non-empty sequence of digits
	 */
    for (;; p++, ndig++)
    {
        c = *p;
        v = base;
        if ('0' <= c && c <= '9')
        {
            v = c - '0';
        }
        else if ('a' <= c && c <= 'z')
        {
            v = c - 'a' + 10;
        }
        else if ('A' <= c && c <= 'Z')
        {
            v = c - 'A' + 10;
        }
        if (v >= base)
        {
            break;
        }
        n = n * base + v;
    }
Return:
    if (ndig == 0)
    {
        p = nptr;
    }
    if (endptr)
    {
        *endptr = (char *)p;
    }
    if (neg)
    {
        return (uvlong)0 - n;
    }
    return n;
}
