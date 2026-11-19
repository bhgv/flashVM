/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 utfrrune support.
 * RU: исходная POSIX-поддержка lib9 utfrrune.
 */
#include "lib9.h"

/* Invalid rune values must not alias a byte after libc character conversion. */


/* [standalone-contract:utfrrune]
 * EN: utfrrune: original POSIX lib9 utfrrune support.
 * RU: utfrrune: исходная POSIX-поддержка lib9 utfrrune.
 * s (char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * c (long c): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: char *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
char *utfrrune(char *s, long c)
{
    long c1;
    Rune r;
    char *s1;

    if (c < 0 || c > Runemax)
    {
        return nil;
    }
    if (c < Runesync) /* not part of utf sequence */
    {
        return strrchr(s, c);
    }

    s1 = 0;
    for (;;)
    {
        c1 = *(uchar *)s;
        if (c1 < Runeself)
        { /* one byte rune */
            if (c1 == 0)
            {
                return s1;
            }
            if (c1 == c)
            {
                s1 = s;
            }
            s++;
            continue;
        }
        c1 = chartorune(&r, s);
        if (r == c)
        {
            s1 = s;
        }
        s += c1;
    }
}
