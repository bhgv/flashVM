/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of dofmt.c; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */
/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 dofmt support.
 * RU: исходная POSIX-поддержка lib9 dofmt.
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


/* [standalone-contract:dofmt]
 * EN: dofmt: original POSIX lib9 dofmt support.
 * RU: dofmt: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * fmt (char *fmt): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int dofmt(Fmt *f, char *fmt)
{
    Rune rune, *rt, *rs;
    int r;
    char *t, *s;
    int n, nfmt;

    nfmt = f->nfmt;
    for (;;)
    {
        if (f->runes)
        {
            rt = f->to;
            rs = f->stop;
            while ((r = *(uchar *)fmt) && r != '%')
            {
                if (r < Runeself)
                {
                    fmt++;
                }
                else
                {
                    fmt += chartorune(&rune, fmt);
                    r = rune;
                }
                FMTRCHAR(f, rt, rs, r);
            }
            fmt++;
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
            while ((r = *(uchar *)fmt) && r != '%')
            {
                if (r < Runeself)
                {
                    FMTCHAR(f, t, s, r);
                    fmt++;
                }
                else
                {
                    n = chartorune(&rune, fmt);
                    if (n > s - t)
                    {
                        t = _fmtflush(f, t, n);
                        if (t != nil)
                        {
                            s = f->stop;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    while (n--)
                    {
                        *t++ = *fmt++;
                    }
                }
            }
            fmt++;
            f->nfmt += t - (char *)f->to;
            f->to = t;
            if (!r)
            {
                return f->nfmt - nfmt;
            }
            f->stop = s;
        }

        fmt = _fmtdispatch(f, fmt, 0);
        if (fmt == nil)
        {
            return -1;
        }
    }
}

/* [standalone-contract:_fmtflush]
 * EN: _fmtflush: original POSIX lib9 dofmt support.
 * RU: _fmtflush: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * t (void *t): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * len (int len): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: void *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: void *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void *_fmtflush(Fmt *f, void *t, int len)
{
    if (f->runes)
    {
        f->nfmt += (Rune *)t - (Rune *)f->to;
    }
    else
    {
        f->nfmt += (char *)t - (char *)f->to;
    }
    f->to = t;
    if (f->flush == 0 || (*f->flush)(f) == 0 || len < 0 || len > (char *)f->stop - (char *)f->to)
    {
        f->stop = f->to;
        return nil;
    }
    return f->to;
}

/*
 * put a formatted block of memory sz bytes long of n runes into the output buffer,
 * left/right justified in a field of at least f->width charactes
 */


/* [standalone-contract:_fmtpad]
 * EN: _fmtpad: original POSIX lib9 dofmt support.
 * RU: _fmtpad: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * n (int n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _fmtpad(Fmt *f, int n)
{
    char *t, *s;
    int i;

    t = f->to;
    s = f->stop;
    for (i = 0; i < n; i++)
    {
        FMTCHAR(f, t, s, ' ');
    }
    f->nfmt += t - (char *)f->to;
    f->to = t;
    return 0;
}

/* [standalone-contract:_rfmtpad]
 * EN: _rfmtpad: original POSIX lib9 dofmt support.
 * RU: _rfmtpad: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * n (int n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _rfmtpad(Fmt *f, int n)
{
    Rune *t, *s;
    int i;

    t = f->to;
    s = f->stop;
    for (i = 0; i < n; i++)
    {
        FMTRCHAR(f, t, s, ' ');
    }
    f->nfmt += t - (Rune *)f->to;
    f->to = t;
    return 0;
}

/* Decode one bounded UTF-8 span; an incomplete tail becomes Runeerror safely. */


/* [standalone-contract:_fmtdecode]
 * EN: _fmtdecode: original POSIX lib9 dofmt support.
 * RU: _fmtdecode: исходная POSIX-поддержка lib9 dofmt.
 * r (Rune *r): Unicode rune value or rune buffer, distinct from a UTF-8 byte count.
 * RU: Unicode rune либо буфер rune, отличный от числа UTF-8 байт.
 * s (char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * size (int size): Requested byte extent; the relevant buffer must cover it.
 * RU: Запрошенная длина в байтах; соответствующий буфер должен её покрывать.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _fmtdecode(Rune *r, char *s, int size)
{
    char tail[UTFmax + 1];

    if (size <= 0)
    {
        return 0;
    }
    if (size >= UTFmax || fullrune(s, size))
    {
        return chartorune(r, s);
    }
    memmove(tail, s, size);
    tail[size] = 0;
    return chartorune(r, tail);
}

/* Copy counted runes without confusing an intentionally unused byte suffix with failure. */


/* [standalone-contract:_fmtcpy]
 * EN: _fmtcpy: original POSIX lib9 dofmt support.
 * RU: _fmtcpy: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * vm (void *vm): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * n (int n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * sz (int sz): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: -1, 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1, 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _fmtcpy(Fmt *f, void *vm, int n, int sz)
{
    Rune *rt, *rs, r;
    char *t, *s, *m, *me;
    ulong fl;
    int nc, w;

    if (n < 0 || sz < 0 || n > sz || (vm == nil && sz != 0))
    {
        return -1;
    }
    m = vm;
    me = m == nil ? nil : m + sz;
    w = f->width;
    fl = f->flags;
    if ((fl & FmtPrec) && n > f->prec)
    {
        n = f->prec;
    }
    if (f->runes)
    {
        if (!(fl & FmtLeft) && _rfmtpad(f, w - n) < 0)
        {
            return -1;
        }
        rt = f->to;
        rs = f->stop;
        for (nc = n; nc > 0 && m < me; nc--)
        {
            m += _fmtdecode(&r, m, me - m);
            FMTRCHAR(f, rt, rs, r);
        }
        f->nfmt += rt - (Rune *)f->to;
        f->to = rt;
        if (nc > 0)
        {
            return -1;
        }
        if (fl & FmtLeft && _rfmtpad(f, w - n) < 0)
        {
            return -1;
        }
    }
    else
    {
        if (!(fl & FmtLeft) && _fmtpad(f, w - n) < 0)
        {
            return -1;
        }
        t = f->to;
        s = f->stop;
        for (nc = n; nc > 0 && m < me; nc--)
        {
            m += _fmtdecode(&r, m, me - m);
            FMTRUNE(f, t, s, r);
        }
        f->nfmt += t - (char *)f->to;
        f->to = t;
        if (nc > 0)
        {
            return -1;
        }
        if (fl & FmtLeft && _fmtpad(f, w - n) < 0)
        {
            return -1;
        }
    }
    return 0;
}

/* Copy a counted Rune span without forming an end pointer for an empty nil span. */


/* [standalone-contract:_fmtrcpy]
 * EN: _fmtrcpy: original POSIX lib9 dofmt support.
 * RU: _fmtrcpy: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * vm (void *vm): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * n (int n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: int; direct result constants in this body: -1, 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1, 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _fmtrcpy(Fmt *f, void *vm, int n)
{
    Rune r, *m, *rt, *rs;
    char *t, *s;
    ulong fl;
    int nc, w;

    if (n < 0 || (vm == nil && n != 0))
    {
        return -1;
    }
    m = vm;
    w = f->width;
    fl = f->flags;
    if ((fl & FmtPrec) && n > f->prec)
    {
        n = f->prec;
    }
    if (f->runes)
    {
        if (!(fl & FmtLeft) && _rfmtpad(f, w - n) < 0)
        {
            return -1;
        }
        rt = f->to;
        rs = f->stop;
        for (nc = n; nc > 0; nc--, m++)
        {
            FMTRCHAR(f, rt, rs, *m);
        }
        f->nfmt += rt - (Rune *)f->to;
        f->to = rt;
        if (fl & FmtLeft && _rfmtpad(f, w - n) < 0)
        {
            return -1;
        }
    }
    else
    {
        if (!(fl & FmtLeft) && _fmtpad(f, w - n) < 0)
        {
            return -1;
        }
        t = f->to;
        s = f->stop;
        for (nc = n; nc > 0; nc--, m++)
        {
            r = *m;
            FMTRUNE(f, t, s, r);
        }
        f->nfmt += t - (char *)f->to;
        f->to = t;
        if (fl & FmtLeft && _fmtpad(f, w - n) < 0)
        {
            return -1;
        }
    }
    return 0;
}

/* fmt out one character */


/* [standalone-contract:_charfmt]
 * EN: _charfmt: original POSIX lib9 dofmt support.
 * RU: _charfmt: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _charfmt(Fmt *f)
