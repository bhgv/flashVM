/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 sysfatal support.
 * RU: исходная POSIX-поддержка lib9 sysfatal.
 */
#include "lib9.h"

/* [standalone-contract:_sysfatalimpl]
 * EN: _sysfatalimpl: original POSIX lib9 sysfatal support.
 * RU: _sysfatalimpl: исходная POSIX-поддержка lib9 sysfatal.
 * fmt (char *fmt): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * arg (va_list arg): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void _sysfatalimpl(char *fmt, va_list arg)
{
    char buf[1024];

    vseprint(buf, buf + sizeof(buf), fmt, arg);
    if (argv0)
    {
        fprint(2, "%s: %s\n", argv0, buf);
    }
    else
    {
        fprint(2, "%s\n", buf);
    }
    exits(buf);
}

void (*_sysfatal)(char *fmt, va_list arg) = _sysfatalimpl;

/* [standalone-contract:sysfatal]
 * EN: sysfatal: original POSIX lib9 sysfatal support.
 * RU: sysfatal: исходная POSIX-поддержка lib9 sysfatal.
 * fmt (char *fmt): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * ... (...): Variadic arguments matching the supplied format.
 * RU: Переменные аргументы, соответствующие переданному format.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void sysfatal(char *fmt, ...)
{
    va_list arg;

    va_start(arg, fmt);
    (*_sysfatal)(fmt, arg);
    va_end(arg);
}
