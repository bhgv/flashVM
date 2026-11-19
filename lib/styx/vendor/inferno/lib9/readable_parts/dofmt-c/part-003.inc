/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of dofmt.c; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */

        *(vlong *)p = f->nfmt;
    }
    else if (fl & FmtLong)
    {
        *(long *)p = f->nfmt;
    }
    else if (fl & FmtByte)
    {
        *(char *)p = f->nfmt;
    }
    else if (fl & FmtShort)
    {
        *(short *)p = f->nfmt;
    }
    else
    {
        *(int *)p = f->nfmt;
    }
    return 0;
}

/* [standalone-contract:_flagfmt]
 * EN: _flagfmt: original POSIX lib9 dofmt support.
 * RU: _flagfmt: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _flagfmt(Fmt *f)
{
    switch (f->r)
    {
    case ',':
        f->flags |= FmtComma;
        break;
    case '-':
        f->flags |= FmtLeft;
        break;
    case '+':
        f->flags |= FmtSign;
        break;
    case '#':
        f->flags |= FmtSharp;
        break;
    case ' ':
        f->flags |= FmtSpace;
        break;
    case 'u':
        f->flags |= FmtUnsigned;
        break;
    case 'h':
        if (f->flags & FmtShort)
        {
            f->flags |= FmtByte;
        }
        f->flags |= FmtShort;
        break;
    case 'l':
        if (f->flags & FmtLong)
        {
            f->flags |= FmtVLong;
        }
        f->flags |= FmtLong;
        break;
    }
    return 1;
}

/* default error format */


/* [standalone-contract:_badfmt]
 * EN: _badfmt: original POSIX lib9 dofmt support.
 * RU: _badfmt: исходная POSIX-поддержка lib9 dofmt.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int _badfmt(Fmt *f)
{
    char x[3];

    x[0] = '%';
    x[1] = f->r;
    x[2] = '%';
    f->prec = 3;
    _fmtcpy(f, x, 3, 3);
    return 0;
}
