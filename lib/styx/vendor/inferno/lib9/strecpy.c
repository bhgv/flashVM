/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 strecpy support.
 * RU: исходная POSIX-поддержка lib9 strecpy.
 */
#include "lib9.h"

/* [standalone-contract:strecpy]
 * EN: strecpy: original POSIX lib9 strecpy support.
 * RU: strecpy: исходная POSIX-поддержка lib9 strecpy.
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
char *strecpy(char *to, char *e, char *from)
{
    if (to >= e)
    {
        return to;
    }
    to = memccpy(to, from, '\0', e - to);
    if (to == nil)
    {
        to = e - 1;
        *to = '\0';
    }
    else
    {
        to--;
    }
    return to;
}
