/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 fmtrune support.
 * RU: исходная POSIX-поддержка lib9 fmtrune.
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

/* [standalone-contract:fmtrune]
 * EN: fmtrune: original POSIX lib9 fmtrune support.
 * RU: fmtrune: исходная POSIX-поддержка lib9 fmtrune.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * r (int r): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int fmtrune(Fmt *f, int r)
{
    Rune *rt;
    char *t;
    int n;

    if (f->runes)
    {
        rt = f->to;
        FMTRCHAR(f, rt, f->stop, r);
        f->to = rt;
        n = 1;
    }
    else
    {
        t = f->to;
        FMTRUNE(f, t, f->stop, r);
        n = t - (char *)f->to;
        f->to = t;
    }
    f->nfmt += n;
    return 0;
}
