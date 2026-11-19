/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 utfnlen support.
 * RU: исходная POSIX-поддержка lib9 utfnlen.
 */
#include "lib9.h"

/* Count complete runes without forming an end pointer beyond the actual string. */


/* [standalone-contract:utfnlen]
 * EN: utfnlen: original POSIX lib9 utfnlen support.
 * RU: utfnlen: исходная POSIX-поддержка lib9 utfnlen.
 * s (char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * m (long m): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int utfnlen(char *s, long m)
{
    int c, consumed;
    long n;
    Rune rune;

    for (n = 0; m > 0; n++)
    {
        c = *(uchar *)s;
        if (c < Runeself)
        {
            if (c == '\0')
            {
                break;
            }
            consumed = 1;
        }
        else
        {
            if (!fullrune(s, m < UTFmax ? (int)m : UTFmax))
            {
                break;
            }
            consumed = chartorune(&rune, s);
        }
        s += consumed;
        m -= consumed;
    }
    return n;
}
