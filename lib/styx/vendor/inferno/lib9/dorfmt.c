/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 dorfmt support.
 * RU: исходная POSIX-поддержка lib9 dorfmt.
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

/* format the output into f->to and return the number of characters fmted  */


/* [standalone-contract:dorfmt]
 * EN: dorfmt: original POSIX lib9 dorfmt support.
 * RU: dorfmt: исходная POSIX-поддержка lib9 dorfmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * fmt (Rune *fmt): Unicode rune value or rune buffer, distinct from a UTF-8 byte count.
 * RU: Unicode rune либо буфер rune, отличный от числа UTF-8 байт.
 * Return: int; direct result constants in this body: -1, 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1, 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int dorfmt(Fmt *f, Rune *fmt)
{
    Rune *rt, *rs;
    int r;
    char *t, *s;
    int nfmt;

    nfmt = f->nfmt;
    for (;;)
    {
        if (f->runes)
        {
            rt = f->to;
            rs = f->stop;
            while ((r = *fmt++) && r != '%')
            {
                FMTRCHAR(f, rt, rs, r);
            }
            f->nfmt += rt - (Rune *)f->to;
            f->to = rt;
            if (!r)
            {
                return f->nfmt - nfmt;
            }
            f->stop = rs;
        }
        else
        {
            t = f->to;
            s = f->stop;
            while ((r = *fmt++) && r != '%')
            {
                FMTRUNE(f, t, f->stop, r);
            }
            f->nfmt += t - (char *)f->to;
            f->to = t;
            if (!r)
            {
                return f->nfmt - nfmt;
            }
            f->stop = s;
        }

        fmt = _fmtdispatch(f, fmt, 1);
        if (fmt == nil)
        {
            return -1;
        }
    }
    return 0; /* not reached */
}
