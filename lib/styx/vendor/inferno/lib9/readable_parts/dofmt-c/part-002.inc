/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of dofmt.c; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */

{
    char x[1];

    x[0] = va_arg(f->args, int);
    f->prec = 1;
    return _fmtcpy(f, x, 1, 1);
}

/* fmt out one rune */


/* [standalone-contract:_runefmt]
 * EN: _runefmt: original POSIX lib9 dofmt support.
 * RU: _runefmt: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _runefmt(Fmt *f)
{
    Rune x[1];

    x[0] = va_arg(f->args, int);
    return _fmtrcpy(f, x, 1);
}

/* public helper routine: fmt out a null terminated string already in hand */


/* [standalone-contract:fmtstrcpy]
 * EN: fmtstrcpy: original POSIX lib9 dofmt support.
 * RU: fmtstrcpy: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * s (char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int fmtstrcpy(Fmt *f, char *s)
{
    uvlong size;
    int p, i;
    if (!s)
    {
        return _fmtcpy(f, "<nil>", 5, 5);
    }
    /* if precision is specified, make sure we don't wander off the end */
    if (f->flags & FmtPrec)
    {
        p = f->prec;
        for (i = 0; i < p; i++)
        {
            if (s[i] == 0)
            {
                break;
            }
        }
        return _fmtcpy(f, s, utfnlen(s, i), i); /* byte precision omits incomplete runes */
    }

    size = strlen(s);
    if (size > 0x7fffffffUL)
    {
        return -1;
    }
    return _fmtcpy(f, s, utflen(s), size);
}

/* fmt out a null terminated utf string */


/* [standalone-contract:_strfmt]
 * EN: _strfmt: original POSIX lib9 dofmt support.
 * RU: _strfmt: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _strfmt(Fmt *f)
{
    char *s;

    s = va_arg(f->args, char *);
    return fmtstrcpy(f, s);
}

/* public helper routine: fmt out a null terminated rune string already in hand */


/* [standalone-contract:fmtrunestrcpy]
 * EN: fmtrunestrcpy: original POSIX lib9 dofmt support.
 * RU: fmtrunestrcpy: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * s (Rune *s): Unicode rune value or rune buffer, distinct from a UTF-8 byte count.
 * RU: Unicode rune либо буфер rune, отличный от числа UTF-8 байт.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int fmtrunestrcpy(Fmt *f, Rune *s)
{
    Rune *e;
    uvlong size;
    int n, p;

    if (!s)
    {
        return _fmtcpy(f, "<nil>", 5, 5);
    }
    /* if precision is specified, make sure we don't wander off the end */
    if (f->flags & FmtPrec)
    {
        p = f->prec;
        for (n = 0; n < p; n++)
        {
            if (s[n] == 0)
            {
                break;
            }
        }
    }
    else
    {
        for (e = s; *e; e++)
            ;
        size = e - s;
        if (size > 0x7fffffffUL)
        {
            return -1;
        }
        n = size;
    }
    return _fmtrcpy(f, s, n);
}

/* fmt out a null terminated rune string */


/* [standalone-contract:_runesfmt]
 * EN: _runesfmt: original POSIX lib9 dofmt support.
 * RU: _runesfmt: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _runesfmt(Fmt *f)
{
    Rune *s;

    s = va_arg(f->args, Rune *);
    return fmtrunestrcpy(f, s);
}

/* fmt a % */


/* [standalone-contract:_percentfmt]
 * EN: _percentfmt: original POSIX lib9 dofmt support.
 * RU: _percentfmt: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _percentfmt(Fmt *f)
{
    Rune x[1];

    x[0] = f->r;
    f->prec = 1;
    return _fmtrcpy(f, x, 1);
}

/* fmt an integer */
/* Leave room for grouped 64-bit binary digits and negate magnitudes unsigned. */


/* [standalone-contract:_ifmt]
 * EN: _ifmt: original POSIX lib9 dofmt support.
 * RU: _ifmt: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _ifmt(Fmt *f)
{
    char buf[128], *p, *conv;
    uvlong vu;
    ulong u;
    int neg, base, i, n, fl, w, isv;

    neg = 0;
    fl = f->flags;
    isv = 0;
    vu = 0;
    u = 0;
    if (f->r == 'p')
    {
        u = (ulong)va_arg(f->args, void *);
        f->r = 'x';
        fl |= FmtUnsigned;
    }
    else if (fl & FmtVLong)
    {
        isv = 1;
        if (fl & FmtUnsigned)
        {
            vu = va_arg(f->args, uvlong);
        }
        else
        {
            vu = va_arg(f->args, vlong);
        }
    }
    else if (fl & FmtLong)
    {
        if (fl & FmtUnsigned)
        {
            u = va_arg(f->args, ulong);
        }
        else
        {
            u = va_arg(f->args, long);
        }
    }
    else if (fl & FmtByte)
    {
        if (fl & FmtUnsigned)
        {
            u = (uchar)va_arg(f->args, int);
        }
        else
        {
            u = (char)va_arg(f->args, int);
        }
    }
    else if (fl & FmtShort)
    {
        if (fl & FmtUnsigned)
        {
            u = (ushort)va_arg(f->args, int);
        }
        else
        {
            u = (short)va_arg(f->args, int);
        }
    }
    else
    {
        if (fl & FmtUnsigned)
        {
            u = va_arg(f->args, uint);
        }
        else
        {
            u = va_arg(f->args, int);
        }
    }
    conv = "0123456789abcdef";
    switch (f->r)
    {
    case 'd':
        base = 10;
        break;
    case 'x':
        base = 16;
        break;
    case 'X':
        base = 16;
        conv = "0123456789ABCDEF";
        break;
    case 'b':
        base = 2;
        break;
    case 'o':
        base = 8;
        break;
    default:
        return -1;
    }
    if (!(fl & FmtUnsigned))
    {
        if (isv && (vlong)vu < 0)
        {
            vu = (uvlong)0 - vu;
            neg = 1;
        }
        else if (!isv && (long)u < 0)
        {
            u = (ulong)0 - u;
            neg = 1;
        }
    }
    p = buf + sizeof buf - 1;
    n = 0;
    if (isv)
    {
        while (vu)
        {
            i = vu % base;
            vu /= base;
            if ((fl & FmtComma) && n % 4 == 3)
            {
                *p-- = ',';
                n++;
            }
            *p-- = conv[i];
            n++;
        }
    }
    else
    {
        while (u)
        {
            i = u % base;
            u /= base;
            if ((fl & FmtComma) && n % 4 == 3)
            {
                *p-- = ',';
                n++;
            }
            *p-- = conv[i];
            n++;
        }
    }
    if (n == 0)
    {
        *p-- = '0';
        n = 1;
    }
    for (w = f->prec; n < w && p > buf + 3; n++)
    {
        *p-- = '0';
    }
    if (neg || (fl & (FmtSign | FmtSpace)))
    {
        n++;
    }
    if (fl & FmtSharp)
    {
        if (base == 16)
        {
            n += 2;
        }
        else if (base == 8)
        {
            if (p[1] == '0')
            {
                fl &= ~FmtSharp;
            }
            else
            {
                n++;
            }
        }
    }
    if ((fl & FmtZero) && !(fl & FmtLeft))
    {
        for (w = f->width; n < w && p > buf + 3; n++)
        {
            *p-- = '0';
        }
        f->width = 0;
    }
    if (fl & FmtSharp)
    {
        if (base == 16)
        {
            *p-- = f->r;
        }
        if (base == 16 || base == 8)
        {
            *p-- = '0';
        }
    }
    if (neg)
    {
        *p-- = '-';
    }
    else if (fl & FmtSign)
    {
        *p-- = '+';
    }
    else if (fl & FmtSpace)
    {
        *p-- = ' ';
    }
    f->flags &= ~FmtPrec;
    return _fmtcpy(f, p + 1, n, n);
}

/* [standalone-contract:_countfmt]
 * EN: _countfmt: original POSIX lib9 dofmt support.
 * RU: _countfmt: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _countfmt(Fmt *f)
{
    void *p;
    ulong fl;

    fl = f->flags;
    p = va_arg(f->args, void *);
    if (fl & FmtVLong)
    {
