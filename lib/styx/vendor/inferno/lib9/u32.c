/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 u32 support.
 * RU: исходная POSIX-поддержка lib9 u32.
 */
#include <lib9.h>

static char base32[] = "23456789abcdefghijkmnpqrstuvwxyz";

/*
 * Decode complete base32 byte groups. Validate the whole input and capacity
 * before writing, so malformed text or short buffers leave dest unchanged.
 */


/* [standalone-contract:dec32]
 * EN: dec32: original POSIX lib9 u32 support.
 * RU: dec32: исходная POSIX-поддержка lib9 u32.
 * dest (uchar *dest): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * ndest (int ndest): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * src (char *src): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * nsrc (int nsrc): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int dec32(uchar *dest, int ndest, char *src, int nsrc)
{
    uvlong required;
    uint value;
    int i, bits, n, tail;
    char *digit;

    if (nsrc < 0 || ndest < 0)
    {
        return -1;
    }
    tail = nsrc % 8;
    if (tail == 1 || tail == 3 || tail == 6)
    {
        return -1;
    }
    required = (uvlong)nsrc * 5 / 8;
    if (required > (uvlong)ndest)
    {
        return -1;
    }
    for (i = 0; i < nsrc; i++)
    {
        if (src[i] == 0 || strchr(base32, (uchar)src[i]) == nil)
        {
            return -1;
        }
    }

    value = 0;
    bits = 0;
    n = 0;
    for (i = 0; i < nsrc; i++)
    {
        digit = strchr(base32, (uchar)src[i]);
        value = (value << 5) | (digit - base32);
        bits += 5;
        if (bits >= 8)
        {
            bits -= 8;
            dest[n++] = value >> bits;
        }
    }
    return n;
}

/* Encode every tail length, reserving space for the terminating NUL. */


/* [standalone-contract:enc32]
 * EN: enc32: original POSIX lib9 u32 support.
 * RU: enc32: исходная POSIX-поддержка lib9 u32.
 * dest (char *dest): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * ndest (int ndest): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * src (uchar *src): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * nsrc (int nsrc): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: -1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: -1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
int enc32(char *dest, int ndest, uchar *src, int nsrc)
{
    uvlong required;
    uint value;
    int i, bits, n;

    if (nsrc < 0 || ndest <= 0)
    {
        return -1;
    }
    required = ((uvlong)nsrc * 8 + 4) / 5;
    if (required >= (uvlong)ndest)
    {
        return -1;
    }

    value = 0;
    bits = 0;
    n = 0;
    for (i = 0; i < nsrc; i++)
    {
        value = (value << 8) | src[i];
        bits += 8;
        while (bits >= 5)
        {
            bits -= 5;
            dest[n++] = base32[(value >> bits) & 31];
        }
    }
    if (bits)
    {
        dest[n++] = base32[(value << (5 - bits)) & 31];
    }
    dest[n] = 0;
    return n;
}
