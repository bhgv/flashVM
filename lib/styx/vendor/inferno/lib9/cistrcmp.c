/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 cistrcmp support.
 * RU: исходная POSIX-поддержка lib9 cistrcmp.
 */
#include "lib9.h"

/* Compare unsigned bytes with ASCII case folding and symmetric NUL ordering. */


/* [standalone-contract:cistrcmp]
 * EN: cistrcmp: original POSIX lib9 cistrcmp support.
 * RU: cistrcmp: исходная POSIX-поддержка lib9 cistrcmp.
 * s1 (char *s1): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * s2 (char *s2): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int cistrcmp(char *s1, char *s2)
{
    int c1, c2;

    for (;;)
    {
        c1 = *(uchar *)s1++;
        c2 = *(uchar *)s2++;
        if (c1 >= 'A' && c1 <= 'Z')
        {
            c1 += 'a' - 'A';
        }
        if (c2 >= 'A' && c2 <= 'Z')
        {
            c2 += 'a' - 'A';
        }
        if (c1 != c2 || c1 == 0)
        {
            return c1 - c2;
        }
    }
}
