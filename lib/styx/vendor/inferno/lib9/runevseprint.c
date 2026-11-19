/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 runevseprint support.
 * RU: исходная POSIX-поддержка lib9 runevseprint.
 */
#include "lib9.h"
#include "fmtdef.h"

/* Validate aligned native Rune endpoints before pointer formation or format dispatch. */


/* [standalone-contract:runevseprint]
 * EN: runevseprint: original POSIX lib9 runevseprint support.
 * RU: runevseprint: исходная POSIX-поддержка lib9 runevseprint.
 * buf (Rune *buf): Unicode rune value or rune buffer, distinct from a UTF-8 byte count.
 * RU: Unicode rune либо буфер rune, отличный от числа UTF-8 байт.
 * e (Rune *e): Unicode rune value or rune buffer, distinct from a UTF-8 byte count.
 * RU: Unicode rune либо буфер rune, отличный от числа UTF-8 байт.
 * fmt (char *fmt): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * args (va_list args): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: Rune *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: Rune *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
Rune *runevseprint(Rune *buf, Rune *e, char *fmt, va_list args)
{
    Fmt f;

    if (buf == nil || (uintptr)buf == (uintptr)-1 || fmt == nil || (uintptr)fmt == (uintptr)-1 ||
        (uintptr)e <= (uintptr)buf || (uintptr)buf % sizeof(Rune) || (uintptr)e % sizeof(Rune))
    {
        return nil;
    }
    f.runes = 1;
    f.start = buf;
    f.to = buf;
    f.stop = e - 1;
    f.flush = nil;
    f.farg = nil;
    f.nfmt = 0;
    va_copy(f.args, args);
    dofmt(&f, fmt);
    va_end(f.args);
    *(Rune *)f.to = '\0';
    return f.to;
}

/* Check a Rune count before constructing the bounded endpoint. */


/* [standalone-contract:runevsnprint]
 * EN: runevsnprint: original POSIX lib9 runevseprint support.
 * RU: runevsnprint: исходная POSIX-поддержка lib9 runevseprint.
 * buf (Rune *buf): Unicode rune value or rune buffer, distinct from a UTF-8 byte count.
 * RU: Unicode rune либо буфер rune, отличный от числа UTF-8 байт.
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
int runevsnprint(Rune *buf, int len, char *fmt, va_list args)
{
    Rune *end;

    if (len <= 0 || buf == nil || (uintptr)buf % sizeof(Rune) ||
        (uintptr)len > ((uintptr)-1 - (uintptr)buf) / sizeof(Rune))
    {
        return -1;
    }
    end = runevseprint(buf, buf + len, fmt, args);
    if (end == nil)
    {
        return -1;
    }
    return end - buf;
}
