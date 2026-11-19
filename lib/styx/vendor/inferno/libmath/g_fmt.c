/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: libmath/g_fmt implementation and native state.
 * RU: реализация libmath/g_fmt и нативное состояние.
 */
/****************************************************************
 *
 * The author of this software is David M. Gay.
 *
 * Copyright (c) 1991, 1996 by Lucent Technologies.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 *
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHOR NOR LUCENT MAKES ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 ***************************************************************/

/* g_fmt(buf,x) stores the closest decimal approximation to x in buf;
 * it suffices to declare buf
 *	char buf[32];
 */

#ifdef __cplusplus
extern "C"
{
#endif
extern char *dtoa(double, int, int, int *, int *, char **);
extern char *g_fmt(char *, double, int);
extern void freedtoa(char *);
#ifdef __cplusplus
}
#endif


/* [standalone-contract:g_fmt]
 * EN: g_fmt: libmath/g_fmt implementation and native state.
 * RU: g_fmt: реализация libmath/g_fmt и нативное состояние.
 * b (register char *b): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * x (double x): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * echr (int echr): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
char *g_fmt(register char *b, double x, int echr)
{
    register int i, k;
    register char *s;
    int decpt, j, sign;
    char *b0, *s0, *se;

    b0 = b;
#ifdef IGNORE_ZERO_SIGN
    if (!x)
    {
        *b++ = '0';
        *b = 0;
        goto done;
    }
#endif
    s = s0 = dtoa(x, 0, 0, &decpt, &sign, &se);
    if (sign)
    {
        *b++ = '-';
    }
    if (decpt == 9999) /* Infinity or Nan */
    {
        while (*b++ = *s++)
            ;
        goto done0;
    }
    if (decpt <= -4 || decpt > se - s + 5)
    {
        *b++ = *s++;
        if (*s)
        {
            *b++ = '.';
            while (*b = *s++)
            {
                b++;
            }
        }
        *b++ = echr;
        /* sprintf(b, "%+.2d", decpt - 1); */
        if (--decpt < 0)
        {
            *b++ = '-';
            decpt = -decpt;
        }
        else
        {
            *b++ = '+';
        }
        for (j = 2, k = 10; 10 * k <= decpt; j++, k *= 10)
            ;
        for (;;)
        {
            i = decpt / k;
            *b++ = i + '0';
            if (--j <= 0)
            {
                break;
            }
            decpt -= i * k;
            decpt *= 10;
        }
        *b = 0;
    }
    else if (decpt <= 0)
    {
        *b++ = '.';
        for (; decpt < 0; decpt++)
        {
            *b++ = '0';
        }
        while (*b++ = *s++)
            ;
    }
    else
    {
        while (*b = *s++)
        {
            b++;
            if (--decpt == 0 && *s)
            {
                *b++ = '.';
            }
        }
        for (; decpt > 0; decpt--)
        {
            *b++ = '0';
        }
        *b = 0;
    }
done0:
    freedtoa(s0);
#ifdef IGNORE_ZERO_SIGN
done:
#endif
    return b0;
}
