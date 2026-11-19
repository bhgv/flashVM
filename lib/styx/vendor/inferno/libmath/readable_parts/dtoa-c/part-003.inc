/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of dtoa.c; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */

        i = b->wds = (x[1] = z) ? 2 : 1;
    }
    else
    {
        k = lo0bits(&z);
        x[0] = z;
        i = b->wds = 1;
        k += 32;
    }
#ifndef Sudden_Underflow
    if (de)
    {
#endif
        *e = de - Bias - (P - 1) + k;
        *bits = P - k;
#ifndef Sudden_Underflow
    }
    else
    {
        *e = de - Bias - (P - 1) + 1 + k;
        *bits = 32 * i - hi0bits(x[i - 1]);
    }
#endif
    return b;
}

#undef d0
#undef d1

/* [standalone-contract:ratio]
 * EN: ratio: original decimal conversion with checked integer limbs and synchronized public entry points.
 * RU: ratio: исходное decimal-преобразование с проверенными limbs и синхронизированными публичными входами.
 * a (Bigint *a): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * b (Bigint *b): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: double; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: double; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static double ratio(Bigint *a, Bigint *b)
{
    double da, db;
    int k, ka, kb;

    da = b2d(a, &ka);
    db = b2d(b, &kb);
    k = ka - kb + 32 * (a->wds - b->wds);
    if (k > 0)
    {
        word0(da) += k * Exp_msk1;
    }
    else
    {
        k = -k;
        word0(db) += k * Exp_msk1;
    }
    return da / db;
}

static const double tens[] = {1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,  1e8,  1e9,  1e10, 1e11,
                              1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22};

static const double bigtens[] = {1e16, 1e32, 1e64, 1e128, 1e256};

static const double tinytens[] = {1e-16, 1e-32, 1e-64, 1e-128, 9007199254740992.e-256};

/* The factor of 2^53 in tinytens[4] helps us avoid setting the underflow */
/* flag unnecessarily.  It leads to a song and dance at the end of strtod. */
#define Scale_Bit 0x10
#define n_bigtens 5

#define NAN_WORD0 0x7ff80000

#define NAN_WORD1 0

/* [standalone-contract:match]
 * EN: match: original decimal conversion with checked integer limbs and synchronized public entry points.
 * RU: match: исходное decimal-преобразование с проверенными limbs и синхронизированными публичными входами.
 * sp (const char **sp): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * t (char *t): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: 0, 1. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0, 1. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int match(const char **sp, char *t)
{
    int c, d;
    const char *s = *sp;

    while (d = *t++)
    {
        if ((c = *++s) >= 'A' && c <= 'Z')
        {
            c += 'a' - 'A';
        }
        if (c != d)
        {
            return 0;
        }
    }
    *sp = s + 1;
    return 1;
}

/* [standalone-contract:limbo_decimal_strtod_unlocked]
 * EN: limbo_decimal_strtod_unlocked: original decimal conversion with checked integer limbs and synchronized public entry points.
 * RU: limbo_decimal_strtod_unlocked: исходное decimal-преобразование с проверенными limbs и синхронизированными публичными входами.
 * s00 (const char *s00): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * se (char **se): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: double; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: double; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
double limbo_decimal_strtod_unlocked(const char *s00, char **se)
{
    int scale;
    int bb2, bb5, bbe, bd2, bd5, bbbits, bs2, c, dsign, e, e1, esign, i, j, k, nd, nd0, nf, nz, nz0, sign;
    const char *s, *s0, *s1;
    double aadj, aadj1, adj, rv, rv0;
    int L;
    unsigned int y, z;
    Bigint *bb, *bb1, *bd, *bd0, *bs, *delta;
    sign = nz0 = nz = 0;
    rv = 0.;
    for (s = s00;; s++)
    {
        switch (*s)
        {
        case '-':
            sign = 1;
            /* no break */
        case '+':
            if (*++s)
            {
                goto break2;
            }
            /* no break */
        case 0:
            s = s00;
            goto ret;
        case '\t':
        case '\n':
        case '\v':
        case '\f':
        case '\r':
        case ' ':
            continue;
        default:
            goto break2;
        }
    }
break2:
    if (*s == '0')
    {
        nz0 = 1;
        while (*++s == '0')
            ;
        if (!*s)
        {
            goto ret;
        }
    }
    s0 = s;
    y = z = 0;
    for (nd = nf = 0; (c = *s) >= '0' && c <= '9'; nd++, s++)
    {
        if (nd < 9)
        {
            y = 10 * y + c - '0';
        }
        else if (nd < 16)
        {
            z = 10 * z + c - '0';
        }
    }
    nd0 = nd;
    if (c == '.')
    {
        c = *++s;
        if (!nd)
        {
            for (; c == '0'; c = *++s)
            {
                nz++;
            }
            if (c > '0' && c <= '9')
            {
                s0 = s;
                nf += nz;
                nz = 0;
                goto have_dig;
            }
            goto dig_done;
        }
        for (; c >= '0' && c <= '9'; c = *++s)
        {
        have_dig:
            nz++;
            if (c -= '0')
            {
                nf += nz;
                for (i = 1; i < nz; i++)
                {
                    if (nd++ < 9)
                    {
                        y *= 10;
                    }
                    else if (nd <= DBL_DIG + 1)
                    {
                        z *= 10;
                    }
                }
                if (nd++ < 9)
                {
                    y = 10 * y + c;
                }
                else if (nd <= DBL_DIG + 1)
                {
                    z = 10 * z + c;
                }
                nz = 0;
            }
        }
    }
dig_done:
    e = 0;
    if (c == 'e' || c == 'E')
    {
        if (!nd && !nz && !nz0)
        {
            s = s00;
            goto ret;
        }
        s00 = s;
        esign = 0;
        switch (c = *++s)
        {
        case '-':
            esign = 1;
        case '+':
            c = *++s;
        }
        if (c >= '0' && c <= '9')
        {
            while (c == '0')
            {
                c = *++s;
            }
            if (c > '0' && c <= '9')
            {
                L = c - '0';
                s1 = s;
                while ((c = *++s) >= '0' && c <= '9')
                {
                    L = 10 * L + c - '0';
                }
                if (s - s1 > 8 || L > 19999)
                {
                    /* Avoid confusion from exponents
					 * so large that e might overflow.
					 */
                    e = 19999; /* safe for 16 bit ints */
                }
                else
                {
                    e = (int)L;
                }
                if (esign)
                {
                    e = -e;
                }
            }
            else
            {
                e = 0;
            }
        }
        else
        {
            s = s00;
        }
    }
    if (!nd)
    {
        if (!nz && !nz0)
        {
            /* Check for Nan and Infinity */
            switch (c)
            {
            case 'i':
            case 'I':
                if (match(&s, "nfinity"))
                {
                    word0(rv) = 0x7ff00000;
                    word1(rv) = 0;
                    goto ret;
                }
                break;
            case 'n':
            case 'N':
                if (match(&s, "an"))
                {
                    word0(rv) = NAN_WORD0;
                    word1(rv) = NAN_WORD1;
                    goto ret;
                }
            }
            s = s00;
        }
        goto ret;
    }
    e1 = e -= nf;

    /* Now we have nd0 digits, starting at s0, followed by a
	 * decimal point, followed by nd-nd0 digits.  The number we're
	 * after is the integer represented by those digits times
	 * 10**e */

    if (!nd0)
    {
        nd0 = nd;
    }
    k = nd < DBL_DIG + 1 ? nd : DBL_DIG + 1;
    rv = y;
    if (k > 9)
    {
        rv = tens[k - 9] * rv + z;
    }
    bd0 = 0;
    if (nd <= DBL_DIG && FLT_ROUNDS == 1)
    {
        if (!e)
        {
            goto ret;
        }
        if (e > 0)
        {
            if (e <= Ten_pmax)
            {
                /* rv = */ rounded_product(rv, tens[e]);
                goto ret;
            }
            i = DBL_DIG - nd;
            if (e <= Ten_pmax + i)
            {
                /* A fancier test would sometimes let us do
				 * this for larger i values.
				 */
                e -= i;
                rv *= tens[i];
                /* rv = */ rounded_product(rv, tens[e]);
                goto ret;
            }
        }
        else if (e >= -Ten_pmax)
        {
            /* rv = */ rounded_quotient(rv, tens[-e]);
            goto ret;
        }
    }
    e1 += nd - k;

    scale = 0;

    /* Get starting approximation = rv * 10**e1 */

    if (e1 > 0)
    {
        if (i = e1 & 15)
        {
            rv *= tens[i];
        }
        if (e1 &= ~15)
        {
            if (e1 > DBL_MAX_10_EXP)
            {
            ovfl:
                /* Can't trust HUGE_VAL */
                word0(rv) = Exp_mask;
                word1(rv) = 0;
                if (bd0)
                {
                    goto retfree;
                }
                goto ret;
            }
            if (e1 >>= 4)
            {
                for (j = 0; e1 > 1; j++, e1 >>= 1)
                {
                    if (e1 & 1)
                    {
                        rv *= bigtens[j];
                    }
                }
                /* The last multiplication could overflow. */
                word0(rv) -= P * Exp_msk1;
                rv *= bigtens[j];
                if ((z = word0(rv) & Exp_mask) > Exp_msk1 * (DBL_MAX_EXP + Bias - P))
                {
                    goto ovfl;
                }
                if (z > Exp_msk1 * (DBL_MAX_EXP + Bias - 1 - P))
                {
                    /* set to largest number */
                    /* (Can't trust DBL_MAX) */
                    word0(rv) = Big0;
                    word1(rv) = Big1;
                }
                else
                {
                    word0(rv) += P * Exp_msk1;
                }
            }
        }
    }
    else if (e1 < 0)
    {
        e1 = -e1;
        if (i = e1 & 15)
        {
