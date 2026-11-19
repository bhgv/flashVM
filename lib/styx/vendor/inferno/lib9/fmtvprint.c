/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 fmtvprint support.
 * RU: исходная POSIX-поддержка lib9 fmtvprint.
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

/*
 * format a string into the output buffer
 * designed for formats which themselves call fmt,
 * but ignore any width flags
 */


/* [standalone-contract:fmtvprint]
 * EN: fmtvprint: original POSIX lib9 fmtvprint support.
 * RU: fmtvprint: исходная POSIX-поддержка lib9 fmtvprint.
 * f (Fmt *f): Active original formatter context, including its output cursor and conversion state.
 * RU: Активный исходный formatter-контекст, включая cursor вывода и состояние преобразования.
 * fmt (char *fmt): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * args (va_list args): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int fmtvprint(Fmt *f, char *fmt, va_list args)
{
    va_list va;
    int n;

    f->flags = 0;
    f->width = 0;
    f->prec = 0;
    va_copy(va, f->args);
    va_end(f->args);
    va_copy(f->args, args);
    n = dofmt(f, fmt);
    f->flags = 0;
    f->width = 0;
    f->prec = 0;
    va_end(f->args);
    va_copy(f->args, va);
    va_end(va);
    if (n >= 0)
    {
        return 0;
    }
    return n;
}
