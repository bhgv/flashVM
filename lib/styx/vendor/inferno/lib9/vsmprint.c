/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 vsmprint support.
 * RU: исходная POSIX-поддержка lib9 vsmprint.
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

/* Validate the owned byte view before growth or pointer-difference narrowing. */


/* [standalone-contract:fmtStrValid]
 * EN: fmtStrValid: original POSIX lib9 vsmprint support.
 * RU: fmtStrValid: исходная POSIX-поддержка lib9 vsmprint.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int fmtStrValid(Fmt *f)
{
    uintptr start, capacity;

    if (f == nil || f->runes || f->start == nil || f->to == nil || f->stop == nil)
    {
        return 0;
    }
    start = (uintptr)f->start;
    capacity = (uintptr)f->farg;
    if (capacity == 0 || capacity > 0x7fffffff || start > (uintptr)-1 - capacity ||
        (uintptr)f->stop != start + capacity - 1 || (uintptr)f->to < start || (uintptr)f->to > (uintptr)f->stop)
    {
        return 0;
    }
    return 1;
}

/* Capture the cursor before geometric growth and clear the owner if realloc fails. */


/* [standalone-contract:fmtStrFlush]
 * EN: fmtStrFlush: original POSIX lib9 vsmprint support.
 * RU: fmtStrFlush: исходная POSIX-поддержка lib9 vsmprint.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int fmtStrFlush(Fmt *f)
{
    char *old, *grown;
    uintptr capacity;
    int n, used;

    if (!fmtStrValid(f))
    {
        return 0;
    }
    capacity = (uintptr)f->farg;
    if (capacity == 0x7fffffff)
    {
        return 0;
    }
    /* Keep the existing first 256-byte expansion for ordinary short strings. */
    if (capacity < 256)
    {
        n = capacity + 256;
    }
    else
    {
        n = capacity > 0x7fffffff / 2 ? 0x7fffffff : capacity * 2;
    }
    old = f->start;
    used = (uintptr)f->to - (uintptr)old;
    grown = realloc(old, n);
    if (grown == nil)
    {
        free(old);
        f->start = nil;
        f->to = nil;
        f->stop = nil;
        f->farg = nil;
        return 0;
    }
    f->start = grown;
    f->to = grown + used;
    f->stop = grown + n - 1;
    f->farg = (void *)(uintptr)n;
    return 1;
}

/* Initialize one owned byte string; a failed allocation leaves an empty formatter. */


/* [standalone-contract:fmtstrinit]
 * EN: fmtstrinit: original POSIX lib9 vsmprint support.
 * RU: fmtstrinit: исходная POSIX-поддержка lib9 vsmprint.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: -1, 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1, 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int fmtstrinit(Fmt *f)
{
    int n;

    if (f == nil)
    {
        return -1;
    }
    memset(f, 0, sizeof(*f));
    n = 32;
    f->start = malloc(n);
    if (f->start == nil)
    {
        return -1;
    }
    f->to = f->start;
    f->stop = (char *)f->start + n - 1;
    f->flush = fmtStrFlush;
    f->farg = (void *)(uintptr)n;
    return 0;
}

/* Return a complete allocated string, releasing the owner on every formatting failure. */


/* [standalone-contract:vsmprint]
 * EN: vsmprint: original POSIX lib9 vsmprint support.
 * RU: vsmprint: исходная POSIX-поддержка lib9 vsmprint.
 * fmt (char *fmt): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * args (va_list args): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: char *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
char *vsmprint(char *fmt, va_list args)
{
    Fmt f;
    char *result;
    int n;

    if (fmt == nil || (uintptr)fmt == (uintptr)-1 || fmtstrinit(&f) < 0)
    {
        return nil;
    }
    va_copy(f.args, args);
    n = dofmt(&f, fmt);
    va_end(f.args);
    if (n < 0)
    {
        free(f.start);
        return nil;
    }
    result = fmtstrflush(&f);
    if (result == nil)
    {
        free(f.start);
    }
    return result;
}

/* Grow an owned Rune buffer in Rune units, retaining its cursor across realloc. */


/* [standalone-contract:runeStrGrow]
 * EN: runeStrGrow: original POSIX lib9 vsmprint support.
 * RU: runeStrGrow: исходная POSIX-поддержка lib9 vsmprint.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int runeStrGrow(Fmt *f)
{
    uintptr start, capacity, used, next;
    Rune *grown;

    if (f == nil || f->runes != 1 || f->start == nil)
    {
        return 0;
    }
    start = (uintptr)f->start;
    capacity = (uintptr)f->farg;
    if (capacity == 0 || capacity >= 0x7fffffff / sizeof(Rune) || start % sizeof(Rune) ||
        start > (uintptr)-1 - capacity * sizeof(Rune) || (uintptr)f->stop != start + (capacity - 1) * sizeof(Rune) ||
        (uintptr)f->to < start || (uintptr)f->to > (uintptr)f->stop || ((uintptr)f->to - start) % sizeof(Rune))
    {
        return 0;
    }
    used = ((uintptr)f->to - start) / sizeof(Rune);
    next = capacity > (0x7fffffff / sizeof(Rune)) / 2 ? 0x7fffffff / sizeof(Rune) : capacity * 2;
    grown = realloc(f->start, next * sizeof(Rune));
    if (grown == nil)
    {
        free(f->start);
        f->start = nil;
        f->to = nil;
        f->stop = nil;
        f->farg = nil;
        return 0;
    }
    f->start = grown;
    f->to = grown + used;
    f->stop = grown + next - 1;
    f->farg = (void *)next;
    return 1;
}

/* Initialize an allocated Rune formatter with a reserved terminator slot. */


/* [standalone-contract:runefmtstrinit]
 * EN: runefmtstrinit: original POSIX lib9 vsmprint support.
 * RU: runefmtstrinit: исходная POSIX-поддержка lib9 vsmprint.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * Return: int; direct result constants in this body: -1, 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1, 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int runefmtstrinit(Fmt *f)
{
    if (f == nil)
    {
        return -1;
    }
    memset(f, 0, sizeof(*f));
    f->runes = 1;
    f->start = malloc(32 * sizeof(Rune));
    if (f->start == nil)
    {
        return -1;
    }
    f->to = f->start;
    f->stop = (Rune *)f->start + 31;
    f->farg = (void *)(uintptr)32;
    f->flush = runeStrGrow;
    return 0;
}

/* Format directly in Rune mode so counts and custom conversions retain their semantics. */


/* [standalone-contract:runevsmprint]
 * EN: runevsmprint: original POSIX lib9 vsmprint support.
 * RU: runevsmprint: исходная POSIX-поддержка lib9 vsmprint.
 * fmt (char *fmt): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * args (va_list args): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: Rune *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: Rune *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
Rune *runevsmprint(char *fmt, va_list args)
{
    Fmt f;
    Rune *result;
    int n;

    if (fmt == nil || (uintptr)fmt == (uintptr)-1 || runefmtstrinit(&f) < 0)
    {
        return nil;
    }
    va_copy(f.args, args);
    n = dofmt(&f, fmt);
    va_end(f.args);
    if (n < 0)
    {
        free(f.start);
        return nil;
    }
    result = runefmtstrflush(&f);
    if (result == nil)
    {
        free(f.start);
    }
    return result;
}
