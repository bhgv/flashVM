/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of dtoa.c; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */
/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original decimal conversion with checked integer limbs and synchronized public entry points.
 * RU: исходное decimal-преобразование с проверенными limbs и синхронизированными публичными входами.
 */
/* derived from /netlib/fp/dtoa.c assuming IEEE, Standard C */
/* kudos to dmg@bell-labs.com, gripes to ehg@bell-labs.com */
#include "lib9.h"
#include <limits.h>
#if UINT_MAX != 0xffffffffU
#error dtoa_requires_32_bit_unsigned_int
#endif

#ifdef __APPLE__
#pragma clang diagnostic ignored "-Wlogical-op-parentheses"
#pragma clang diagnostic ignored "-Wparentheses"
#endif
#define ACQUIRE_DTOA_LOCK(n) /*nothing*/
#define FREE_DTOA_LOCK(n)    /*nothing*/

/* let's provide reasonable defaults for usual implementation of IEEE f.p. */
#ifndef DBL_DIG
#define DBL_DIG 15
#endif
#ifndef DBL_MAX_10_EXP
#define DBL_MAX_10_EXP 308
#endif
#ifndef DBL_MAX_EXP
#define DBL_MAX_EXP 1024
#endif
#ifndef FLT_RADIX
#define FLT_RADIX 2
#endif
#ifndef FLT_ROUNDS
#define FLT_ROUNDS 1
#endif
#ifndef Storeinc
#define Storeinc(a, b, c) (*a++ = (unsigned int)(b) << 16 | (unsigned int)(c) & 0xffffU)
#endif

#define Sign_Extend(a, b)                                                                                              \
    if (b < 0)                                                                                                         \
        a |= 0xffff0000;

#ifdef USE_FPdbleword
#define word0(x) ((FPdbleword *)&x)->hi
#define word1(x) ((FPdbleword *)&x)->lo
#else
#ifdef __LITTLE_ENDIAN
#define word0(x) ((unsigned int *)&x)[1]
#define word1(x) ((unsigned int *)&x)[0]
#else
#define word0(x) ((unsigned int *)&x)[0]
#define word1(x) ((unsigned int *)&x)[1]
#endif
#endif

/* #define P DBL_MANT_DIG */
/* Ten_pmax = floor(P*log(2)/log(5)) */
/* Bletch = (highest power of 2 < DBL_MAX_10_EXP) / 16 */
/* Quick_max = floor((P-1)*log(FLT_RADIX)/log(10) - 1) */
/* Int_max = floor(P*log(FLT_RADIX)/log(10) - 1) */

#define Exp_shift 20
#define Exp_shift1 20
#define Exp_msk1 0x100000
#define Exp_msk11 0x100000
#define Exp_mask 0x7ff00000
#define P 53
#define Bias 1023
#define Emin (-1022)
#define Exp_1 0x3ff00000
#define Exp_11 0x3ff00000
#define Ebits 11
#define Frac_mask 0xfffff
#define Frac_mask1 0xfffff
#define Ten_pmax 22
#define Bletch 0x10
#define Bndry_mask 0xfffff
#define Bndry_mask1 0xfffff
#define LSB 1
#define Sign_bit 0x80000000
#define Log2P 1
#define Tiny0 0
#define Tiny1 1
#define Quick_max 14
#define Int_max 14
#define Infinite(x) (word0(x) == 0x7ff00000) /* sufficient test for here */
#define Avoid_Underflow

#define rounded_product(a, b) a *= b
#define rounded_quotient(a, b) a /= b

#define Big0 (Frac_mask1 | Exp_msk1 * (DBL_MAX_EXP + Bias - 1))
#define Big1 0xffffffff

#define Kmax 15

struct Bigint
{
    struct Bigint *next;
    int k, maxwds, sign, wds;
    unsigned int x[1];
};

typedef struct Bigint Bigint;

static Bigint *freelist[Kmax + 1];

/* [standalone-contract:Balloc]
 * EN: Balloc: original decimal conversion with checked integer limbs and synchronized public entry points.
 * RU: Balloc: исходное decimal-преобразование с проверенными limbs и синхронизированными публичными входами.
 * k (int k): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: Bigint *; direct result constants in this body: nil. Forwarded results preserve the called API's status.
 * Возврат: Bigint *; прямые константы результата тела: nil. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static Bigint *Balloc(int k)
{
    int x;
    Bigint *rv;

    ACQUIRE_DTOA_LOCK(0);
    if (rv = freelist[k])
    {
        freelist[k] = rv->next;
    }
    else
    {
        x = 1 << k;
        rv = (Bigint *)malloc(sizeof(Bigint) + (x - 1) * sizeof(unsigned int));
        if (rv == nil)
        {
            return nil;
        }
        rv->k = k;
        rv->maxwds = x;
    }
    FREE_DTOA_LOCK(0);
    rv->sign = rv->wds = 0;
    return rv;
}

/* [standalone-contract:Bfree]
 * EN: Bfree: original decimal conversion with checked integer limbs and synchronized public entry points.
 * RU: Bfree: исходное decimal-преобразование с проверенными limbs и синхронизированными публичными входами.
 * v (Bigint *v): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static void Bfree(Bigint *v)
{
    if (v)
    {
        ACQUIRE_DTOA_LOCK(0);
        v->next = freelist[v->k];
        freelist[v->k] = v;
        FREE_DTOA_LOCK(0);
    }
}

#define Bcopy(x, y) memcpy((char *)&x->sign, (char *)&y->sign, y->wds * sizeof(int) + 2 * sizeof(int))

/* [standalone-contract:multadd]
 * EN: multadd: original decimal conversion with checked integer limbs and synchronized public entry points.
 * RU: multadd: исходное decimal-преобразование с проверенными limbs и синхронизированными публичными входами.
 * b (Bigint *b): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * m (int m): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * a (int a): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: Bigint *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: Bigint *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static Bigint *multadd(Bigint *b, int m, int a) /* multiply by m and add a */
{
    int i, wds;
    unsigned int *x, y;
    unsigned int xi, z;
    Bigint *b1;

    wds = b->wds;
    x = b->x;
    i = 0;
    do
    {
        xi = *x;
        y = (xi & 0xffff) * m + a;
        z = (xi >> 16) * m + (y >> 16);
        a = (int)(z >> 16);
        *x++ = (z << 16) + (y & 0xffff);
    } while (++i < wds);
    if (a)
    {
        if (wds >= b->maxwds)
        {
            b1 = Balloc(b->k + 1);
            Bcopy(b1, b);
            Bfree(b);
            b = b1;
        }
        b->x[wds++] = a;
        b->wds = wds;
    }
    return b;
}

/* [standalone-contract:s2b]
 * EN: s2b: original decimal conversion with checked integer limbs and synchronized public entry points.
 * RU: s2b: исходное decimal-преобразование с проверенными limbs и синхронизированными публичными входами.
 * s (const char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * nd0 (int nd0): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * nd (int nd): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * y9 (unsigned int y9): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: Bigint *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: Bigint *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static Bigint *s2b(const char *s, int nd0, int nd, unsigned int y9)
{
    Bigint *b;
    int i, k;
    int x, y;

    x = (nd + 8) / 9;
    for (k = 0, y = 1; x > y; y <<= 1, k++)
        ;
    b = Balloc(k);
    b->x[0] = y9;
    b->wds = 1;

    i = 9;
    if (9 < nd0)
    {
        s += 9;
        do
        {
            b = multadd(b, 10, *s++ - '0');
        } while (++i < nd0);
        s++;
    }
    else
    {
        s += 10;
    }
    for (; i < nd; i++)
    {
        b = multadd(b, 10, *s++ - '0');
    }
    return b;
}

/* [standalone-contract:hi0bits]
 * EN: hi0bits: original decimal conversion with checked integer limbs and synchronized public entry points.
 * RU: hi0bits: исходное decimal-преобразование с проверенными limbs и синхронизированными публичными входами.
 * x (register unsigned int x): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: int; direct result constants in this body: 32. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 32. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int hi0bits(register unsigned int x)
{
    register int k = 0;

    if (!(x & 0xffff0000))
    {
        k = 16;
        x <<= 16;
    }
    if (!(x & 0xff000000))
    {
        k += 8;
        x <<= 8;
    }
    if (!(x & 0xf0000000))
    {
        k += 4;
        x <<= 4;
    }
    if (!(x & 0xc0000000))
    {
        k += 2;
        x <<= 2;
    }
    if (!(x & 0x80000000))
    {
        k++;
        if (!(x & 0x40000000))
        {
            return 32;
        }
    }
    return k;
}

/* [standalone-contract:lo0bits]
 * EN: lo0bits: original decimal conversion with checked integer limbs and synchronized public entry points.
 * RU: lo0bits: исходное decimal-преобразование с проверенными limbs и синхронизированными публичными входами.
 * y (unsigned int *y): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: 0, 1, 2, 32. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1, 2, 32. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int lo0bits(unsigned int *y)
{
    register int k;
    register unsigned int x = *y;

    if (x & 7)
    {
        if (x & 1)
        {
            return 0;
        }
        if (x & 2)
        {
            *y = x >> 1;
            return 1;
        }
        *y = x >> 2;
        return 2;
    }
    k = 0;
    if (!(x & 0xffff))
    {
        k = 16;
        x >>= 16;
    }
    if (!(x & 0xff))
    {
        k += 8;
        x >>= 8;
    }
    if (!(x & 0xf))
    {
        k += 4;
        x >>= 4;
    }
    if (!(x & 0x3))
    {
        k += 2;
        x >>= 2;
    }
    if (!(x & 1))
    {
        k++;
        x >>= 1;
        if (!x & 1)
        {
            return 32;
        }
    }
    *y = x;
    return k;
}

/* [standalone-contract:i2b]
 * EN: i2b: original decimal conversion with checked integer limbs and synchronized public entry points.
 * RU: i2b: исходное decimal-преобразование с проверенными limbs и синхронизированными публичными входами.
 * i (int i): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: Bigint *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: Bigint *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static Bigint *i2b(int i)
{
    Bigint *b;

    b = Balloc(1);
    b->x[0] = i;
    b->wds = 1;
    return b;
}

/* [standalone-contract:mult]
 * EN: mult: original decimal conversion with checked integer limbs and synchronized public entry points.
 * RU: mult: исходное decimal-преобразование с проверенными limbs и синхронизированными публичными входами.
 * a (Bigint *a): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * b (Bigint *b): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: Bigint *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: Bigint *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static Bigint *mult(Bigint *a, Bigint *b)
{
    Bigint *c;
    int k, wa, wb, wc;
    unsigned int carry, y, z;
    unsigned int *x, *xa, *xae, *xb, *xbe, *xc, *xc0;
    unsigned int z2;

    if (a->wds < b->wds)
    {
        c = a;
        a = b;
        b = c;
    }
    k = a->k;
    wa = a->wds;
    wb = b->wds;
    wc = wa + wb;
    if (wc > a->maxwds)
    {
        k++;
    }
    c = Balloc(k);
    for (x = c->x, xa = x + wc; x < xa; x++)
    {
        *x = 0;
