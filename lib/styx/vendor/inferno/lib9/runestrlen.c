/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 runestrlen support.
 * RU: исходная POSIX-поддержка lib9 runestrlen.
 */
#include "lib9.h"

/* Keep the counter as wide as the public length result on LP64 hosts. */


/* [standalone-contract:runestrlen]
 * EN: runestrlen: original POSIX lib9 runestrlen support.
 * RU: runestrlen: исходная POSIX-поддержка lib9 runestrlen.
 * s (Rune *s): Unicode rune value or rune buffer, distinct from a UTF-8 byte count.
 * RU: Unicode rune либо буфер rune, отличный от числа UTF-8 байт.
 * Return: long; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: long; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
long runestrlen(Rune *s)
{
    long i;

    i = 0;
    while (*s++)
    {
        i++;
    }
    return i;
}
