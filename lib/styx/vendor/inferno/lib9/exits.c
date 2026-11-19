/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 exits support.
 * RU: исходная POSIX-поддержка lib9 exits.
 */
#include "lib9.h"

/* [standalone-contract:exits]
 * EN: exits: original POSIX lib9 exits support.
 * RU: exits: исходная POSIX-поддержка lib9 exits.
 * s (char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void exits(char *s)
{
    if (s == 0 || *s == 0)
    {
        exit(0);
    }
    exit(1);
}

/* [standalone-contract:_exits]
 * EN: _exits: original POSIX lib9 exits support.
 * RU: _exits: исходная POSIX-поддержка lib9 exits.
 * s (char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void _exits(char *s)
{
    if (s == 0 || *s == 0)
    {
        _exit(0);
    }
    _exit(1);
}
