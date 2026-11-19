/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 utflen support.
 * RU: исходная POSIX-поддержка lib9 utflen.
 */
#include "lib9.h"

/* [standalone-contract:utflen]
 * EN: utflen: original POSIX lib9 utflen support.
 * RU: utflen: исходная POSIX-поддержка lib9 utflen.
 * s (char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int utflen(char *s)
{
    int c;
    long n;
    Rune rune;

    n = 0;
    for (;;)
    {
        c = *(uchar *)s;
        if (c < Runeself)
        {
            if (c == 0)
            {
                return n;
            }
            s++;
        }
        else
        {
            s += chartorune(&rune, s);
        }
        n++;
    }
}
