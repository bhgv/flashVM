/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 cistrstr support.
 * RU: исходная POSIX-поддержка lib9 cistrstr.
 */
#include "lib9.h"

/* Match ASCII-folded byte prefixes without narrowing a native string length. */


/* [standalone-contract:cistrstr]
 * EN: cistrstr: original POSIX lib9 cistrstr support.
 * RU: cistrstr: исходная POSIX-поддержка lib9 cistrstr.
 * s (char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * sub (char *sub): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: char *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
char *cistrstr(char *s, char *sub)
{
    char *p, *q;
    int c, csub;

    if (*sub == 0)
    {
        return s;
    }
    for (; *s != 0; s++)
    {
        p = s;
        q = sub;
        for (;;)
        {
            csub = *(uchar *)q++;
            if (csub == 0)
            {
                return s;
            }
            c = *(uchar *)p++;
            if (c == 0)
            {
                return nil;
            }
            if (c >= 'A' && c <= 'Z')
            {
                c += 'a' - 'A';
            }
            if (csub >= 'A' && csub <= 'Z')
            {
                csub += 'a' - 'A';
            }
            if (c != csub)
            {
                break;
            }
        }
    }
    return nil;
}
