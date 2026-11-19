/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of dtoa.c; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */
    }
    xa = a->x;
    xae = xa + wa;
    xb = b->x;
    xbe = xb + wb;
    xc0 = c->x;
    for (; xb < xbe; xb++, xc0++)
    {
        if (y = *xb & 0xffff)
        {
            x = xa;
            xc = xc0;
            carry = 0;
            do
            {
                z = (*x & 0xffff) * y + (*xc & 0xffff) + carry;
                carry = z >> 16;
                z2 = (*x++ >> 16) * y + (*xc >> 16) + carry;
                carry = z2 >> 16;
                Storeinc(xc, z2, z);
            } while (x < xae);
            *xc = carry;
        }
        if (y = *xb >> 16)
        {
            x = xa;
            xc = xc0;
            carry = 0;
            z2 = *xc;
            do
            {
                z = (*x & 0xffff) * y + (*xc >> 16) + carry;
                carry = z >> 16;
                Storeinc(xc, z, z2);
                z2 = (*x++ >> 16) * y + (*xc & 0xffff) + carry;
                carry = z2 >> 16;
            } while (x < xae);
            *xc = z2;
        }
    }
    for (xc0 = c->x, xc = xc0 + wc; wc > 0 && !*--xc; --wc)
        ;
    c->wds = wc;
    return c;
}

static Bigint *p5s;

/* [standalone-contract:pow5mult]
 * EN: pow5mult: original decimal conversion with checked integer limbs and synchronized public entry points.
 * RU: pow5mult: исходное decimal-преобразование с проверенными limbs и синхронизированными публичными входами.
 * b (Bigint *b): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * k (int k): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: Bigint *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: Bigint *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static Bigint *pow5mult(Bigint *b, int k)
{
    Bigint *b1, *p5, *p51;
    int i;
    static int p05[3] = {5, 25, 125};

    if (i = k & 3)
    {
        b = multadd(b, p05[i - 1], 0);
    }

    if (!(k >>= 2))
    {
        return b;
    }
    if (!(p5 = p5s))
    {
        /* first time */
        ACQUIRE_DTOA_LOCK(1);
        if (!(p5 = p5s))
        {
            p5 = p5s = i2b(625);
            p5->next = 0;
        }
        FREE_DTOA_LOCK(1);
    }
    for (;;)
    {
        if (k & 1)
        {
            b1 = mult(b, p5);
            Bfree(b);
            b = b1;
        }
        if (!(k >>= 1))
        {
            break;
        }
        if (!(p51 = p5->next))
        {
            ACQUIRE_DTOA_LOCK(1);
            if (!(p51 = p5->next))
            {
                p51 = p5->next = mult(p5, p5);
                p51->next = 0;
            }
            FREE_DTOA_LOCK(1);
        }
        p5 = p51;
    }
    return b;
}

/* [standalone-contract:lshift]
 * EN: lshift: original decimal conversion with checked integer limbs and synchronized public entry points.
 * RU: lshift: исходное decimal-преобразование с проверенными limbs и синхронизированными публичными входами.
 * b (Bigint *b): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * k (int k): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: Bigint *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: Bigint *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static Bigint *lshift(Bigint *b, int k)
{
    int i, k1, n, n1;
    Bigint *b1;
    unsigned int *x, *x1, *xe, z;

    n = k >> 5;
    k1 = b->k;
    n1 = n + b->wds + 1;
    for (i = b->maxwds; n1 > i; i <<= 1)
    {
        k1++;
    }
    b1 = Balloc(k1);
    x1 = b1->x;
    for (i = 0; i < n; i++)
    {
        *x1++ = 0;
    }
    x = b->x;
    xe = x + b->wds;
    if (k &= 0x1f)
    {
        k1 = 32 - k;
        z = 0;
        do
        {
            *x1++ = *x << k | z;
            z = *x++ >> k1;
        } while (x < xe);
        if (*x1 = z)
        {
            ++n1;
        }
    }
    else
    {
        do
        {
            *x1++ = *x++;
        } while (x < xe);
    }
    b1->wds = n1 - 1;
    Bfree(b);
    return b1;
}

/* [standalone-contract:cmp]
 * EN: cmp: original decimal conversion with checked integer limbs and synchronized public entry points.
 * RU: cmp: исходное decimal-преобразование с проверенными limbs и синхронизированными публичными входами.
 * a (Bigint *a): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * b (Bigint *b): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int cmp(Bigint *a, Bigint *b)
{
    unsigned int *xa, *xa0, *xb, *xb0;
    int i, j;

    i = a->wds;
    j = b->wds;
    if (i -= j)
    {
        return i;
    }
    xa0 = a->x;
    xa = xa0 + j;
    xb0 = b->x;
    xb = xb0 + j;
    for (;;)
    {
        if (*--xa != *--xb)
        {
            return *xa < *xb ? -1 : 1;
        }
        if (xa <= xa0)
        {
            break;
        }
    }
    return 0;
}

/* [standalone-contract:diff]
 * EN: diff: original decimal conversion with checked integer limbs and synchronized public entry points.
 * RU: diff: исходное decimal-преобразование с проверенными limbs и синхронизированными публичными входами.
 * a (Bigint *a): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * b (Bigint *b): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: Bigint *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: Bigint *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static Bigint *diff(Bigint *a, Bigint *b)
{
    Bigint *c;
    int i, wa, wb;
    int borrow, y; /* We need signed shifts here. */
    unsigned int *xa, *xae, *xb, *xbe, *xc;
    int z;

    i = cmp(a, b);
    if (!i)
    {
        c = Balloc(0);
        c->wds = 1;
        c->x[0] = 0;
        return c;
    }
    if (i < 0)
    {
        c = a;
        a = b;
        b = c;
        i = 1;
    }
    else
    {
        i = 0;
    }
    c = Balloc(a->k);
    c->sign = i;
    wa = a->wds;
    xa = a->x;
    xae = xa + wa;
    wb = b->wds;
    xb = b->x;
    xbe = xb + wb;
    xc = c->x;
    borrow = 0;
    do
    {
        y = (*xa & 0xffff) - (*xb & 0xffff) + borrow;
        borrow = y >> 16;
        Sign_Extend(borrow, y);
        z = (*xa++ >> 16) - (*xb++ >> 16) + borrow;
        borrow = z >> 16;
        Sign_Extend(borrow, z);
        Storeinc(xc, z, y);
    } while (xb < xbe);
    while (xa < xae)
    {
        y = (*xa & 0xffff) + borrow;
        borrow = y >> 16;
        Sign_Extend(borrow, y);
        z = (*xa++ >> 16) + borrow;
        borrow = z >> 16;
        Sign_Extend(borrow, z);
        Storeinc(xc, z, y);
    }
    while (!*--xc)
    {
        wa--;
    }
    c->wds = wa;
    return c;
}

/* [standalone-contract:ulp]
 * EN: ulp: original decimal conversion with checked integer limbs and synchronized public entry points.
 * RU: ulp: исходное decimal-преобразование с проверенными limbs и синхронизированными публичными входами.
 * x (double x): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: double; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: double; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static double ulp(double x)
{
    register int L;
    double a;

    L = (word0(x) & Exp_mask) - (P - 1) * Exp_msk1;
#ifndef Sudden_Underflow
    if (L > 0)
    {
#endif
        word0(a) = L;
        word1(a) = 0;
#ifndef Sudden_Underflow
    }
    else
    {
        L = -L >> Exp_shift;
        if (L < Exp_shift)
        {
            word0(a) = 0x80000 >> L;
            word1(a) = 0;
        }
        else
        {
            word0(a) = 0;
            L -= Exp_shift;
            word1(a) = L >= 31 ? 1 : 1 << 31 - L;
        }
    }
#endif
    return a;
}

/* [standalone-contract:b2d]
 * EN: b2d: original decimal conversion with checked integer limbs and synchronized public entry points.
 * RU: b2d: исходное decimal-преобразование с проверенными limbs и синхронизированными публичными входами.
 * a (Bigint *a): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * e (int *e): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: double; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: double; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static double b2d(Bigint *a, int *e)
{
    unsigned int *xa, *xa0, w, y, z;
    int k;
    double d;
#define d0 word0(d)
#define d1 word1(d)

    xa0 = a->x;
    xa = xa0 + a->wds;
    y = *--xa;
    k = hi0bits(y);
    *e = 32 - k;
    if (k < Ebits)
    {
        d0 = Exp_1 | y >> Ebits - k;
        w = xa > xa0 ? *--xa : 0;
        d1 = y << (32 - Ebits) + k | w >> Ebits - k;
        goto ret_d;
    }
    z = xa > xa0 ? *--xa : 0;
    if (k -= Ebits)
    {
        d0 = Exp_1 | y << k | z >> 32 - k;
        y = xa > xa0 ? *--xa : 0;
        d1 = z << k | y >> 32 - k;
    }
    else
    {
        d0 = Exp_1 | y;
        d1 = z;
    }
ret_d:
#undef d0
#undef d1
    return d;
}

/* [standalone-contract:d2b]
 * EN: d2b: original decimal conversion with checked integer limbs and synchronized public entry points.
 * RU: d2b: исходное decimal-преобразование с проверенными limbs и синхронизированными публичными входами.
 * d (double d): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * e (int *e): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * bits (int *bits): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: Bigint *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: Bigint *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static Bigint *d2b(double d, int *e, int *bits)
{
    Bigint *b;
    int de, i, k;
    unsigned int *x, y, z;
#define d0 word0(d)
#define d1 word1(d)

    b = Balloc(1);
    x = b->x;

    z = d0 & Frac_mask;
    d0 &= 0x7fffffff; /* clear sign bit, which we ignore */
#ifdef Sudden_Underflow
    de = (int)(d0 >> Exp_shift);
    z |= Exp_msk11;
#else
    if (de = (int)(d0 >> Exp_shift))
    {
        z |= Exp_msk1;
    }
#endif
    if (y = d1)
    {
        if (k = lo0bits(&y))
        {
            x[0] = y | z << 32 - k;
            z >>= k;
        }
        else
        {
            x[0] = y;
        }
