/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 runestrchr support.
 * RU: исходная POSIX-поддержка lib9 runestrchr.
 */
#include "lib9.h"

/* [standalone-contract:runestrchr]
 * EN: runestrchr: original POSIX lib9 runestrchr support.
 * RU: runestrchr: исходная POSIX-поддержка lib9 runestrchr.
 * s (Rune *s): Unicode rune value or rune buffer, distinct from a UTF-8 byte count.
 * RU: Unicode rune либо буфер rune, отличный от числа UTF-8 байт.
 * c (Rune c): Unicode rune value or rune buffer, distinct from a UTF-8 byte count.
 * RU: Unicode rune либо буфер rune, отличный от числа UTF-8 байт.
 * Return: Rune *; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: Rune *; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
Rune *runestrchr(Rune *s, Rune c)
{
    Rune c0 = c;
    Rune c1;

    if (c == 0)
    {
        while (*s++)
            ;
        return s - 1;
    }

    while (c1 = *s++)
    {
        if (c1 == c0)
        {
            return s - 1;
        }
    }
    return 0;
}
