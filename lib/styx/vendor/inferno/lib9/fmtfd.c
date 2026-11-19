/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 fmtfd support.
 * RU: исходная POSIX-поддержка lib9 fmtfd.
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

/* Flush a validated byte view and retain the formatter's total character count. */


/* [standalone-contract:fmtfdflush]
 * EN: fmtfdflush: original POSIX lib9 fmtfd support.
 * RU: fmtfdflush: исходная POSIX-поддержка lib9 fmtfd.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int fmtfdflush(Fmt *f)
{
    if (_fmtFdFlush(f) <= 0)
    {
        return -1;
    }
    return f->nfmt;
}

/* Reject invalid spans before changing the caller's formatter; empty buffers remain valid. */


/* [standalone-contract:fmtfdinit]
 * EN: fmtfdinit: original POSIX lib9 fmtfd support.
 * RU: fmtfdinit: исходная POSIX-поддержка lib9 fmtfd.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * fd (int fd): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * buf (char *buf): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * size (int size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * Return: int; direct result constants in this body: -1, 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1, 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int fmtfdinit(Fmt *f, int fd, char *buf, int size)
{
    if (f == nil || buf == nil || (uintptr)buf == (uintptr)-1 || size < 0 || (uintptr)buf > (uintptr)-1 - (uintptr)size)
    {
        return -1;
    }
    f->runes = 0;
    f->start = buf;
    f->to = buf;
    f->stop = buf + size;
    f->flush = _fmtFdFlush;
    f->farg = (void *)(uintptr)fd;
    f->nfmt = 0;
    return 0;
}
