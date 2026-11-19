/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 errstr-Plan9 support.
 * RU: исходная POSIX-поддержка lib9 errstr-Plan9.
 */
#include "lib9.h"

/* [standalone-contract:oserrstr]
 * EN: oserrstr: original POSIX lib9 errstr-Plan9 support.
 * RU: oserrstr: исходная POSIX-поддержка lib9 errstr-Plan9.
 * buf (char *buf): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * nerr (uint nerr): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void oserrstr(char *buf, uint nerr)
{
    *buf = 0;
    errstr(buf, nerr);
}
