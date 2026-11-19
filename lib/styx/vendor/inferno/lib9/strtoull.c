/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 strtoull support.
 * RU: исходная POSIX-поддержка lib9 strtoull.
 */
#include "lib9.h"

#define UVLONG_MAX (~(uvlong)0)

/* Consume all digits and saturate only beyond the full unsigned 64-bit range. */


/* [standalone-contract:strtoull]
 * EN: strtoull: original POSIX lib9 strtoull support.
 * RU: strtoull: исходная POSIX-поддержка lib9 strtoull.
 * nptr (const char *nptr): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * endptr (char **endptr): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * base (int base): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: uvlong; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: uvlong; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
uvlong strtoull(const char *nptr, char **endptr, int base)
{
    const char *p;
    uvlong n, m, last;
    int c, ovfl, v, neg, ndig;

    p = nptr;
    neg = 0;
    n = 0;
    ndig = 0;
    ovfl = 0;

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
        base = 10;
        if (*p == '0')
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
    m = UVLONG_MAX / base;
    last = UVLONG_MAX % base;
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
        if (n > m || (n == m && (uvlong)v > last))
        {
            ovfl = 1;
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
    if (ovfl)
    {
        return UVLONG_MAX;
    }
    if (neg)
    {
        return -n;
    }
    return n;
}
