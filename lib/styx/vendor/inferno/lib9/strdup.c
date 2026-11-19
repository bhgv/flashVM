/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 strdup support.
 * RU: исходная POSIX-поддержка lib9 strdup.
 */
#include "lib9.h"

/* [standalone-contract:strdup]
 * EN: strdup: original POSIX lib9 strdup support.
 * RU: strdup: исходная POSIX-поддержка lib9 strdup.
 * s (const char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: char *; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
char *strdup(const char *s)
{
    char *os;

    os = malloc(strlen(s) + 1);
    if (os == 0)
    {
        return 0;
    }
    return strcpy(os, s);
}
