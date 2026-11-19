/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 rerrstr support.
 * RU: исходная POSIX-поддержка lib9 rerrstr.
 */
#include "lib9.h"

/* [standalone-contract:rerrstr]
 * EN: rerrstr: original POSIX lib9 rerrstr support.
 * RU: rerrstr: исходная POSIX-поддержка lib9 rerrstr.
 * buf (char *buf): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * nbuf (uint nbuf): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void rerrstr(char *buf, uint nbuf)
{
    char tmp[ERRMAX];

    tmp[0] = 0;
    errstr(tmp, sizeof tmp);
    utfecpy(buf, buf + nbuf, tmp);
    errstr(tmp, sizeof tmp);
}
