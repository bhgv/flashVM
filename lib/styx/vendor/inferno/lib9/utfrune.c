/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 utfrune support.
 * RU: исходная POSIX-поддержка lib9 utfrune.
 */
#include "lib9.h"

/* Invalid rune values must not alias a byte after libc character conversion. */


/* [standalone-contract:utfrune]
 * EN: utfrune: original POSIX lib9 utfrune support.
 * RU: utfrune: исходная POSIX-поддержка lib9 utfrune.
 * s (char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * c (long c): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: char *; direct result constants in this body: 0, nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: 0, nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
char *utfrune(char *s, long c)
{
    long c1;
    Rune r;
    int n;

    if (c < 0 || c > Runemax)
    {
        return nil;
    }
    if (c < Runesync) /* not part of utf sequence */
    {
        return strchr(s, c);
    }

    for (;;)
    {
        c1 = *(uchar *)s;
        if (c1 < Runeself)
        { /* one byte rune */
            if (c1 == 0)
            {
                return 0;
            }
            if (c1 == c)
            {
                return s;
            }
            s++;
            continue;
        }
        n = chartorune(&r, s);
        if (r == c)
        {
            return s;
        }
        s += n;
    }
}
