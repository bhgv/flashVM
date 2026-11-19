/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 runesmprint support.
 * RU: исходная POSIX-поддержка lib9 runesmprint.
 */
#include "lib9.h"

/* Return an owned Rune string through the shared variadic allocator. */


/* [standalone-contract:runesmprint]
 * EN: runesmprint: original POSIX lib9 runesmprint support.
 * RU: runesmprint: исходная POSIX-поддержка lib9 runesmprint.
 * fmt (char *fmt): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * ... (...): Variadic arguments matching the supplied format.
 * RU: Переменные аргументы, соответствующие переданному format.
 * Return: Rune *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: Rune *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
Rune *runesmprint(char *fmt, ...)
{
    va_list args;
    Rune *p;

    va_start(args, fmt);
    p = runevsmprint(fmt, args);
    va_end(args);
    return p;
}
