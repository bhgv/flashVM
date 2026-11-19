/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 u16 support.
 * RU: исходная POSIX-поддержка lib9 u16.
 */
#include <lib9.h>
static char t16e[] = "0123456789ABCDEF";

/* Decode hexadecimal pairs, retaining the legacy ignored-delimiter behavior. */


/* [standalone-contract:dec16]
 * EN: dec16: original POSIX lib9 u16 support.
 * RU: dec16: исходная POSIX-поддержка lib9 u16.
 * out (uchar *out): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * lim (int lim): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * in (char *in): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * n (int n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int dec16(uchar *out, int lim, char *in, int n)
{
    int c, high, pending, used;

    if (lim <= 0 || n <= 0)
    {
        return 0;
    }
    high = 0;
    pending = 0;
    used = 0;
    while (n > 0 && used < lim)
    {
        n--;
        c = (uchar)*in++;
        if (c >= '0' && c <= '9')
        {
            c -= '0';
        }
        else if (c >= 'a' && c <= 'f')
        {
            c = c - 'a' + 10;
        }
        else if (c >= 'A' && c <= 'F')
        {
            c = c - 'A' + 10;
        }
        else
        {
            continue;
        }
        if (pending)
        {
            out[used++] = (high << 4) | c;
            pending = 0;
        }
        else
        {
            high = c;
            pending = 1;
        }
    }
    return used;
}

/* Reserve a terminator and stop before reading a byte that cannot be encoded. */


/* [standalone-contract:enc16]
 * EN: enc16: original POSIX lib9 u16 support.
 * RU: enc16: исходная POSIX-поддержка lib9 u16.
 * out (char *out): Buffer/cursor used by this algorithm; the description defines direction, bounds and retained views.
 * RU: Буфер/cursor этого алгоритма; направление, границы и удерживаемые views заданы описанием.
 * lim (int lim): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * in (uchar *in): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * n (int n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int enc16(char *out, int lim, uchar *in, int n)
{
    uint c;
    int used;

    if (lim <= 0)
    {
        return 0;
    }
    used = 0;
    while (n > 0 && lim - used > 2)
    {
        n--;
        c = *in++;
        out[used++] = t16e[c >> 4];
        out[used++] = t16e[c & 0xf];
    }
    out[used] = 0;
    return used;
}
