/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 vsnprint support.
 * RU: исходная POSIX-поддержка lib9 vsnprint.
 */
/*
 * The authors of this software are Rob Pike and Ken Thompson.
 *              Copyright (c) 2002 by Lucent Technologies.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHORS NOR LUCENT TECHNOLOGIES MAKE ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 */
#include "lib9.h"
#include "fmtdef.h"

/* Reject invalid output spans before pointer formation or formatter side effects. */


/* [standalone-contract:vsnprint]
 * EN: vsnprint: original POSIX lib9 vsnprint support.
 * RU: vsnprint: исходная POSIX-поддержка lib9 vsnprint.
 * buf (char *buf): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * len (int len): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * fmt (char *fmt): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * args (va_list args): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int vsnprint(char *buf, int len, char *fmt, va_list args)
{
    Fmt f;

    if (len <= 0 || buf == nil || (uintptr)buf == (uintptr)-1 || (uintptr)buf > (uintptr)-1 - (uintptr)len ||
        fmt == nil || (uintptr)fmt == (uintptr)-1)
    {
        return -1;
    }
    f.runes = 0;
    f.start = buf;
    f.to = buf;
    f.stop = buf + len - 1;
    f.flush = nil;
    f.farg = nil;
    f.nfmt = 0;
    va_copy(f.args, args);
    dofmt(&f, fmt);
    va_end(f.args);
    *(char *)f.to = '\0';
    return (char *)f.to - buf;
}
