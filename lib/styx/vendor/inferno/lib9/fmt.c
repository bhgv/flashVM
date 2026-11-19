/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original format conversion registry and native callback publication.
 * RU: исходный реестр formatter и публикация нативных callbacks.
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

enum
{
    Maxfmt = 64
};

typedef struct Convfmt Convfmt;

struct Convfmt
{
    int c;
    volatile Fmts fmt; /* Registry mutex protects callback publication and lookup. */
};

struct
{
    /* lock by calling _fmtlock, _fmtunlock */
    int nfmt;
    Convfmt fmt[Maxfmt];
} fmtalloc;

static Convfmt knownfmt[] = {
    ' ',      _flagfmt, '#',      _flagfmt, '%',       _percentfmt, '+',      _flagfmt,  ',',   _flagfmt, '-',
    _flagfmt, 'C',      _runefmt, 'S',      _runesfmt, 'X',         _ifmt,    'b',       _ifmt, 'c',      _charfmt,
    'd',      _ifmt,    'h',      _flagfmt, 'l',       _flagfmt,    'n',      _countfmt, 'o',   _ifmt,    'p',
    _ifmt,    'r',      errfmt,   's',      _strfmt,   'u',         _flagfmt, 'x',       _ifmt, 0,        nil,
};

int (*doquote)(int);

/* EN: Install validated c/f under the registry mutex; no callback is invoked.
 * RU: Установить проверенные c/f под mutex реестра, не вызывая callback. */


/* [standalone-contract:fmtinstalllocked]
 * EN: fmtinstalllocked: original format conversion registry and native callback publication.
 * RU: fmtinstalllocked: исходный реестр formatter и публикация нативных callbacks.
 * c (int c): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * f (Fmts f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: -1, 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1, 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int fmtinstalllocked(int c, Fmts f)
{
    Convfmt *p, *ep;
    ep = &fmtalloc.fmt[fmtalloc.nfmt];
    for (p = fmtalloc.fmt; p < ep; p++)
    {
        if (p->c == c)
        {
            break;
        }
    }
    if (p == &fmtalloc.fmt[Maxfmt])
    {
        return -1;
    }
    p->fmt = f;
    if (p == ep)
    {
        p->c = c;
        fmtalloc.nfmt++;
    }
    return 0;
}

/* EN: Copy c's callback under the same mutex as publication, returning unlocked.
 * Lazy known formats are registered atomically; full tables retain builtin fallback.
 * RU: Скопировать callback для c под mutex публикации и вернуть без mutex.
 * Ленивые форматы регистрируются атомарно; полный реестр сохраняет builtin fallback. */


/* [standalone-contract:fmtfmt]
 * EN: fmtfmt: original format conversion registry and native callback publication.
 * RU: fmtfmt: исходный реестр formatter и публикация нативных callbacks.
 * c (int c): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: Fmts; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: Fmts; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static Fmts fmtfmt(int c)
{
    Convfmt *p, *ep;
    Fmts result;
    _fmtlock();
    ep = &fmtalloc.fmt[fmtalloc.nfmt];
    for (p = fmtalloc.fmt; p < ep; p++)
    {
        if (p->c == c)
        {
            result = p->fmt;
            _fmtunlock();
            return result;
        }
    }
    for (p = knownfmt; p->c; p++)
    {
        if (p->c == c)
        {
            result = p->fmt;
            fmtinstalllocked(c, result);
            _fmtunlock();
            return result;
        }
    }
    _fmtunlock();
    return _badfmt;
}

/* EN: Validate c/f and publish one registry entry; callback code runs elsewhere.
 * Return the original success/failure status, preserving NULL's bad-format fallback.
 * RU: Проверить c/f и опубликовать запись; callback выполняется снаружи.
 * Вернуть исходный статус, сохранив замену NULL на bad-format callback. */


/* [standalone-contract:fmtinstall]
 * EN: fmtinstall: original format conversion registry and native callback publication.
 * RU: fmtinstall: исходный реестр formatter и публикация нативных callbacks.
 * c (int c): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * f (Fmts f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int fmtinstall(int c, Fmts f)
{
    int result;
    if (c <= 0 || c >= 65536)
    {
        return -1;
    }
    if (!f)
    {
        f = _badfmt;
    }
    _fmtlock();
    result = fmtinstalllocked(c, f);
    _fmtunlock();
    return result;
}

/* Reject widths and precisions that cannot be represented by the format state. */


/* [standalone-contract:_fmtdispatch]
 * EN: _fmtdispatch: original format conversion registry and native callback publication.
 * RU: _fmtdispatch: исходный реестр formatter и публикация нативных callbacks.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * fmt (void *fmt): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * isrunes (int isrunes): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: void *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: void *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void *_fmtdispatch(Fmt *f, void *fmt, int isrunes)
{
    Rune rune, r;
    int i, n;

    f->flags = 0;
    f->width = f->prec = 0;

    for (;;)
    {
        if (isrunes)
        {
            r = *(Rune *)fmt;
            fmt = (Rune *)fmt + 1;
        }
        else
        {
            fmt = (char *)fmt + chartorune(&rune, fmt);
            r = rune;
        }
        f->r = r;
        switch (r)
        {
        case '\0':
            return nil;
        case '.':
            f->flags |= FmtWidth | FmtPrec;
            continue;
        case '0':
            if (!(f->flags & FmtWidth))
            {
                f->flags |= FmtZero;
                continue;
            }
            /* fall through */
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            i = 0;
            while (r >= '0' && r <= '9')
            {
                if (i > (0x7fffffff - (r - '0')) / 10)
                {
                    return nil;
                }
                i = i * 10 + r - '0';
                if (isrunes)
                {
                    r = *(Rune *)fmt;
                    fmt = (Rune *)fmt + 1;
                }
                else
                {
                    r = *(char *)fmt;
                    fmt = (char *)fmt + 1;
                }
            }
            if (isrunes)
            {
                fmt = (Rune *)fmt - 1;
            }
            else
            {
                fmt = (char *)fmt - 1;
            }
        numflag:
            if (f->flags & FmtWidth)
            {
                f->flags |= FmtPrec;
                f->prec = i;
            }
            else
            {
                f->flags |= FmtWidth;
                f->width = i;
            }
            continue;
        case '*':
            i = va_arg(f->args, int);
            if (i < 0)
            {
                if (i == (-0x7fffffff - 1))
                {
                    return nil;
                }
                i = -i;
                f->flags |= FmtLeft;
            }
            goto numflag;
        }
        n = (*fmtfmt(r))(f);
        if (n < 0)
        {
            return nil;
        }
        if (n == 0)
        {
            return fmt;
        }
    }
}
