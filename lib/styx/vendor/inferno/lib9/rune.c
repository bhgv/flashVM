/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: UTF-8 byte/rune conversion and validity.
 * RU: преобразование и проверка UTF-8 byte/rune.
 */
#include "lib9.h"

#define Bit(i) (7 - (i))
/* N 0's preceded by i 1's, T(Bit(2)) is 1100 0000 */
#define T(i) (((1 << (Bit(i) + 1)) - 1) ^ 0xFF)
/* 0000 0000 0000 0111 1111 1111 */
#define RuneX(i) ((1 << (Bit(i) + ((i) - 1) * Bitx)) - 1)

enum
{
    Bitx = Bit(1),

    Tx = T(1),                              /* 1000 0000 */
    Rune1 = (1 << (Bit(0) + 0 * Bitx)) - 1, /* 0000 0000 0000 0000 0111 1111 */

    Maskx = (1 << Bitx) - 1,                /* 0011 1111 */
    Testx = Maskx ^ 0xFF,                   /* 1100 0000 */

    SurrogateMin = 0xD800,
    SurrogateMax = 0xDFFF,

    Bad = Runeerror,
};

/* [standalone-contract:chartorune]
 * EN: chartorune: UTF-8 byte/rune conversion and validity.
 * RU: chartorune: преобразование и проверка UTF-8 byte/rune.
 * rune (Rune *rune): Unicode rune value or rune buffer, distinct from a UTF-8 byte count.
 * RU: Unicode rune либо буфер rune, отличный от числа UTF-8 байт.
 * str (char *str): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int chartorune(Rune *rune, char *str)
{
    int c[UTFmax], i;
    Rune l;

    /*
	 * N character sequence
	 *	00000-0007F => T1
	 *	00080-007FF => T2 Tx
	 *	00800-0FFFF => T3 Tx Tx
	 *	10000-10FFFF => T4 Tx Tx Tx
	 */

    c[0] = *(uchar *)(str);
    if (c[0] < Tx)
    {
        *rune = c[0];
        return 1;
    }
    l = c[0];

    for (i = 1; i < UTFmax; i++)
    {
        c[i] = *(uchar *)(str + i);
        c[i] ^= Tx;
        if (c[i] & Testx)
        {
            goto bad;
        }
        l = (l << Bitx) | c[i];
        if (c[0] < T(i + 2))
        {
            l &= RuneX(i + 1);
            if (i == 1)
            {
                if (c[0] < T(2) || l <= Rune1)
                {
                    goto bad;
                }
            }
            else if (l <= RuneX(i) || l > Runemax)
            {
                goto bad;
            }
            if (i == 2 && SurrogateMin <= l && l <= SurrogateMax)
            {
                goto bad;
            }
            *rune = l;
            return i + 1;
        }
    }

    /*
	 * bad decoding
	 */
bad:
    *rune = Bad;
    return 1;
}

/* [standalone-contract:runetochar]
 * EN: runetochar: UTF-8 byte/rune conversion and validity.
 * RU: runetochar: преобразование и проверка UTF-8 byte/rune.
 * str (char *str): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * rune (Rune *rune): Unicode rune value or rune buffer, distinct from a UTF-8 byte count.
 * RU: Unicode rune либо буфер rune, отличный от числа UTF-8 байт.
 * Return: int; direct result constants in this body: 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int runetochar(char *str, Rune *rune)
{
    int i, j;
    Rune c;

    c = *rune;
    if (c <= Rune1)
    {
        str[0] = c;
        return 1;
    }

    /*
	 * one character sequence
	 *	00000-0007F => 00-7F
	 * two character sequence
	 *	0080-07FF => T2 Tx
	 * three character sequence
	 *	0800-FFFF => T3 Tx Tx
	 * four character sequence (21-bit value)
	 *     10000-1FFFFF => T4 Tx Tx Tx
	 * If the Rune is out of range or a surrogate half,
	 * convert it to the error rune.
	 * Do this test when i==3 because the error rune encodes to three bytes.
	 * Doing it earlier would duplicate work, since an out of range
	 * Rune wouldn't have fit in one or two bytes.
	 */
    for (i = 2; i < UTFmax + 1; i++)
    {
        if (i == 3)
        {
            if (c > Runemax)
            {
                c = Runeerror;
            }
            if (SurrogateMin <= c && c <= SurrogateMax)
            {
                c = Runeerror;
            }
        }
        if (c <= RuneX(i) || i == UTFmax)
        {
            str[0] = T(i) | (c >> (i - 1) * Bitx);
            for (j = 1; j < i; j++)
            {
                str[j] = Tx | ((c >> (i - j - 1) * Bitx) & Maskx);
            }
            return i;
        }
    }
    return UTFmax;
}

/* [standalone-contract:runelen]
 * EN: runelen: UTF-8 byte/rune conversion and validity.
 * RU: runelen: преобразование и проверка UTF-8 byte/rune.
 * c (long c): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int runelen(long c)
{
    Rune rune;
    char str[10];

    rune = c;
    return runetochar(str, &rune);
}

/*
 * Count exactly the bytes runetochar emits, including replacement of invalid
 * code points. Callers use this count to size and expose UTF-8 buffers.
 */


/* [standalone-contract:runenlen]
 * EN: runenlen: UTF-8 byte/rune conversion and validity.
 * RU: runenlen: преобразование и проверка UTF-8 byte/rune.
 * r (Rune *r): Unicode rune value or rune buffer, distinct from a UTF-8 byte count.
 * RU: Unicode rune либо буфер rune, отличный от числа UTF-8 байт.
 * nrune (int nrune): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int runenlen(Rune *r, int nrune)
{
    int nb, i;
    Rune c;

    nb = 0;
    while (nrune > 0)
    {
        nrune--;
        c = *r++;
        if (c > Runemax || (SurrogateMin <= c && c <= SurrogateMax))
        {
            c = Runeerror;
        }
        if (c <= Rune1)
        {
            nb++;
        }
        else
        {
            for (i = 2; i < UTFmax + 1; i++)
            {
                if (c <= RuneX(i) || i == UTFmax)
                {
                    nb += i;
                    break;
                }
            }
        }
    }
    return nb;
}

/* [standalone-contract:fullrune]
 * EN: fullrune: UTF-8 byte/rune conversion and validity.
 * RU: fullrune: преобразование и проверка UTF-8 byte/rune.
 * str (char *str): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * n (int n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int fullrune(char *str, int n)
{
    int i;
    Rune c;

    if (n <= 0)
    {
        return 0;
    }
    c = *(uchar *)str;
    if (c < Tx)
    {
        return 1;
    }
    for (i = 3; i < UTFmax + 1; i++)
    {
        if (c < T(i))
        {
            return n >= i - 1;
        }
    }
    return n >= UTFmax;
}
