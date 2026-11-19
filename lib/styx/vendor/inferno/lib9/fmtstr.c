/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 fmtstr support.
 * RU: исходная POSIX-поддержка lib9 fmtstr.
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

/* Terminate only a valid byte view, retaining caller ownership when validation fails. */


/* [standalone-contract:fmtstrflush]
 * EN: fmtstrflush: original POSIX lib9 fmtstr support.
 * RU: fmtstrflush: исходная POSIX-поддержка lib9 fmtstr.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: char *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
char *fmtstrflush(Fmt *f)
{
    if (f == nil || f->runes || f->start == nil || f->to == nil || f->stop == nil || (uintptr)f->start == (uintptr)-1 ||
        (uintptr)f->to < (uintptr)f->start || (uintptr)f->to > (uintptr)f->stop)
    {
        return nil;
    }
    *(char *)f->to = '\0';
    return f->start;
}

/* Terminate an aligned Rune view without transferring or freeing caller ownership. */


/* [standalone-contract:runefmtstrflush]
 * EN: runefmtstrflush: original POSIX lib9 fmtstr support.
 * RU: runefmtstrflush: исходная POSIX-поддержка lib9 fmtstr.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: Rune *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: Rune *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
Rune *runefmtstrflush(Fmt *f)
{
    if (f == nil || f->runes != 1 || f->start == nil || f->to == nil || f->stop == nil ||
        (uintptr)f->start == (uintptr)-1 || (uintptr)f->start % sizeof(Rune) || (uintptr)f->to % sizeof(Rune) ||
        (uintptr)f->stop % sizeof(Rune) || (uintptr)f->to < (uintptr)f->start || (uintptr)f->to > (uintptr)f->stop)
    {
        return nil;
    }
    *(Rune *)f->to = 0;
    return f->start;
}
