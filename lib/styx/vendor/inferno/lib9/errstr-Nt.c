/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 errstr-Nt support.
 * RU: исходная POSIX-поддержка lib9 errstr-Nt.
 */
#include "lib9.h"
#include <Windows.h>

static char errstring[ERRMAX];

enum
{
    Magic = 0xffffff
};

/* [standalone-contract:winerror]
 * EN: winerror: original POSIX lib9 errstr-Nt support.
 * RU: winerror: исходная POSIX-поддержка lib9 errstr-Nt.
 * e (int e): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * buf (char *buf): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * nerr (uint nerr): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void winerror(int e, char *buf, uint nerr)
{
    int r;
    char buf2[ERRMAX], *p, *q;

    r = FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM, 0, e, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf2, sizeof(buf2), 0);

    if (r == 0)
    {
        snprint(buf2, ERRMAX, "windows error %d", e);
    }

    q = buf2;
    for (p = buf2; *p; p++)
    {
        if (*p == '\r')
        {
            continue;
        }
        if (*p == '\n')
        {
            *q++ = ' ';
        }
        else
        {
            *q++ = *p;
        }
    }
    *q = '\0';
    utfecpy(buf, buf + nerr, buf2);
}

/* [standalone-contract:werrstr]
 * EN: werrstr: original POSIX lib9 errstr-Nt support.
 * RU: werrstr: исходная POSIX-поддержка lib9 errstr-Nt.
 * fmt (char *fmt): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * ... (...): Variadic arguments matching the supplied format.
 * RU: Переменные аргументы, соответствующие переданному format.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void werrstr(char *fmt, ...)
{
    va_list arg;

    va_start(arg, fmt);
    vseprint(errstring, errstring + sizeof(errstring), fmt, arg);
    va_end(arg);
    SetLastError(Magic);
}

/* [standalone-contract:errstr]
 * EN: errstr: original POSIX lib9 errstr-Nt support.
 * RU: errstr: исходная POSIX-поддержка lib9 errstr-Nt.
 * buf (char *buf): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * nerr (uint nerr): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int errstr(char *buf, uint nerr)
{
    DWORD le;

    le = GetLastError();
    if (le == Magic)
    {
        utfecpy(buf, buf + nerr, errstring);
    }
    else
    {
        winerror(le, buf, nerr);
    }
    return 1;
}

/* [standalone-contract:oserrstr]
 * EN: oserrstr: original POSIX lib9 errstr-Nt support.
 * RU: oserrstr: исходная POSIX-поддержка lib9 errstr-Nt.
 * buf (char *buf): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * nerr (uint nerr): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void oserrstr(char *buf, uint nerr)
{
    winerror(GetLastError(), buf, nerr);
}
