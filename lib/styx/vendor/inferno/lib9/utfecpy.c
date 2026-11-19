/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 utfecpy support.
 * RU: исходная POSIX-поддержка lib9 utfecpy.
 */
#include "lib9.h"

/* [standalone-contract:utfecpy]
 * EN: utfecpy: original POSIX lib9 utfecpy support.
 * RU: utfecpy: исходная POSIX-поддержка lib9 utfecpy.
 * to (char *to): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * e (char *e): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * from (char *from): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
char *utfecpy(char *to, char *e, char *from)
{
    char *end;

    if (to >= e)
    {
        return to;
    }
    end = memccpy(to, from, '\0', e - to);
    if (end == nil)
    {
        end = e;
        while (end > to && (*--end & 0xC0) == 0x80)
            ;
        *end = '\0';
    }
    else
    {
        end--;
    }
    return end;
}
