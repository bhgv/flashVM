/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 u64 support.
 * RU: исходная POSIX-поддержка lib9 u64.
 */
#include <lib9.h>

enum
{
    INVAL = 255
};

static uchar t64d[256] = {
    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL,
    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL,
    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, 62,    INVAL, INVAL, INVAL, 63,
    52,    53,    54,    55,    56,    57,    58,    59,    60,    61,    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL,
    INVAL, 0,     1,     2,     3,     4,     5,     6,     7,     8,     9,     10,    11,    12,    13,    14,
    15,    16,    17,    18,    19,    20,    21,    22,    23,    24,    25,    INVAL, INVAL, INVAL, INVAL, INVAL,
    INVAL, 26,    27,    28,    29,    30,    31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
    41,    42,    43,    44,    45,    46,    47,    48,    49,    50,    51,    INVAL, INVAL, INVAL, INVAL, INVAL,
    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL,
    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL,
    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL,
    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL,
    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL,
    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL,
    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL,
    INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL, INVAL};
static char t64e[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Decode tolerated base64 input without touching zero-capacity destinations. */


/* [standalone-contract:dec64]
 * EN: dec64: original POSIX lib9 u64 support.
 * RU: dec64: исходная POSIX-поддержка lib9 u64.
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
int dec64(uchar *out, int lim, char *in, int n)
{
    ulong b24;
    uchar *start, *e;
    int i, c;

    if (lim <= 0 || n <= 0)
    {
        return 0;
    }
    start = out;
    e = out + lim;
    b24 = 0;
    i = 0;
    while (n > 0)
    {
        n--;

        c = t64d[*(uchar *)in++];
        if (c == INVAL)
        {
            continue;
        }
        switch (i)
        {
        case 0:
            b24 = c << 18;
            break;
        case 1:
            b24 |= c << 12;
            break;
        case 2:
            b24 |= c << 6;
            break;
        case 3:
            if (e - out < 3)
            {
                goto exhausted;
            }

            b24 |= c;
            *out++ = b24 >> 16;
            *out++ = b24 >> 8;
            *out++ = b24;
            i = -1;
            break;
        }
        i++;
    }
    switch (i)
    {
    case 2:
        if (e - out < 1)
        {
            goto exhausted;
        }
        *out++ = b24 >> 16;
        break;
    case 3:
        if (e - out < 2)
        {
            goto exhausted;
        }
        *out++ = b24 >> 16;
        *out++ = b24 >> 8;
        break;
    }
exhausted:
    return out - start;
}

/* Reserve the terminator before encoding; no output is possible when lim <= 0. */


/* [standalone-contract:enc64]
 * EN: enc64: original POSIX lib9 u64 support.
 * RU: enc64: исходная POSIX-поддержка lib9 u64.
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
int enc64(char *out, int lim, uchar *in, int n)
{
    int i;
    ulong b24;
    char *start, *e;

    if (lim <= 0 || n < 0)
    {
        return 0;
    }
    start = out;
    e = out + lim;

    for (i = n / 3; i > 0; i--)
    {
        if (e - out <= 4)
        {
            goto exhausted;
        }
        b24 = (*in++) << 16;
        b24 |= (*in++) << 8;
        b24 |= *in++;
        *out++ = t64e[(b24 >> 18)];
        *out++ = t64e[(b24 >> 12) & 0x3f];
        *out++ = t64e[(b24 >> 6) & 0x3f];
        *out++ = t64e[(b24) & 0x3f];
    }

    switch (n % 3)
    {
    case 2:
        if (e - out <= 4)
        {
            goto exhausted;
        }
        b24 = (*in++) << 16;
        b24 |= (*in) << 8;
        *out++ = t64e[(b24 >> 18)];
        *out++ = t64e[(b24 >> 12) & 0x3f];
        *out++ = t64e[(b24 >> 6) & 0x3f];
        *out++ = '=';
        break;
    case 1:
        if (e - out <= 4)
        {
            goto exhausted;
        }
        b24 = (*in) << 16;
        *out++ = t64e[(b24 >> 18)];
        *out++ = t64e[(b24 >> 12) & 0x3f];
        *out++ = '=';
        *out++ = '=';
        break;
    }
exhausted:
    *out = 0;
    return out - start;
}
