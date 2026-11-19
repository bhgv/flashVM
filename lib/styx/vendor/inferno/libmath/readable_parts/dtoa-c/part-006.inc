/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of dtoa.c; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */
    {
        S = mhi = 0;

        d -= 5.;
        if (d > eps)

        {
            goto one_digit;
        }
        if (d < -eps)
        {
            goto no_digits;
        }
        goto fast_failed;
    }
    /* Generate ilim digits, then fix them up. */
    eps *= tens[ilim - 1];
    for (i = 1;; i++, d *= 10.)
    {
        L = d;
        d -= L;
        *s++ = '0' + (int)L;
        if (i == ilim)
        {
            if (d > 0.5 + eps)
            {
                goto bump_up;
            }
            else if (d < 0.5 - eps)
            {
                while (*--s == '0')
                    ;
                s++;
                goto ret1;
            }
            break;
        }
    }
fast_failed:
    s = s0;
    d = d2;
    k = k0;
    ilim = ilim0;
}

/* Do we have a "small" integer? */

if (be >= 0 && k <= Int_max)
{
    /* Yes. */
    ds = tens[k];
    if (ndigits < 0 && ilim <= 0)
    {
        S = mhi = 0;
        if (ilim < 0 || d <= 5 * ds)
        {
            goto no_digits;
        }
        goto one_digit;
    }
    for (i = 1;; i++)
    {
        L = d / ds;
        d -= L * ds;
        *s++ = '0' + (int)L;
        if (i == ilim)
        {
            d += d;
            if (d > ds || d == ds && L & 1)
            {
            bump_up:
                while (*--s == '9')
                {
                    if (s == s0)
                    {
                        k++;
                        *s = '0';
                        break;
                    }
                }
                ++*s++;
            }
            break;
        }
        if (!(d *= 10.))
        {
            break;
        }
    }
    goto ret1;
}

m2 = b2;
m5 = b5;
mhi = mlo = 0;
if (leftright)
{
    if (mode < 2)
    {
        i =
#ifndef Sudden_Underflow
            denorm ? be + (Bias + (P - 1) - 1 + 1) :
#endif
                   1 + P - bbits;
    }
    else
    {
        j = ilim - 1;
        if (m5 >= j)
        {
            m5 -= j;
        }
        else
        {
            s5 += j -= m5;
            b5 += j;
            m5 = 0;
        }
        if ((i = ilim) < 0)
        {
            m2 -= i;
            i = 0;
        }
    }
    b2 += i;
    s2 += i;
    mhi = i2b(1);
}
if (m2 > 0 && s2 > 0)
{
    i = m2 < s2 ? m2 : s2;
    b2 -= i;
    m2 -= i;
    s2 -= i;
}
if (b5 > 0)
{
    if (leftright)
    {
        if (m5 > 0)
        {
            mhi = pow5mult(mhi, m5);
            b1 = mult(mhi, b);
            Bfree(b);
            b = b1;
        }
        if (j = b5 - m5)
        {
            b = pow5mult(b, j);
        }
    }
    else
    {
        b = pow5mult(b, b5);
    }
}
S = i2b(1);
if (s5 > 0)
{
    S = pow5mult(S, s5);
}

/* Check for special case that d is a normalized power of 2. */

spec_case = 0;
if (mode < 2)
{
    if (!word1(d) && !(word0(d) & Bndry_mask)
#ifndef Sudden_Underflow
        && word0(d) & Exp_mask
#endif
    )
    {
        /* The special case */
        b2 += Log2P;
        s2 += Log2P;
        spec_case = 1;
    }
}

/* Arrange for convenient computation of quotients:
	 * shift left if necessary so divisor has 4 leading 0 bits.
	 *
	 * Perhaps we should just compute leading 28 bits of S once
	 * and for all and pass them and a shift to quorem, so it
	 * can do shifts and ors to compute the numerator for q.
	 */
if (i = ((s5 ? 32 - hi0bits(S->x[S->wds - 1]) : 1) + s2) & 0x1f)
{
    i = 32 - i;
}
if (i > 4)
{
    i -= 4;
    b2 += i;
    m2 += i;
    s2 += i;
}
else if (i < 4)
{
    i += 28;
    b2 += i;
    m2 += i;
    s2 += i;
}
if (b2 > 0)
{
    b = lshift(b, b2);
}
if (s2 > 0)
{
    S = lshift(S, s2);
}
if (k_check)
{
    if (cmp(b, S) < 0)
    {
        k--;
        b = multadd(b, 10, 0); /* we botched the k estimate */
        if (leftright)
        {
            mhi = multadd(mhi, 10, 0);
        }
        ilim = ilim1;
    }
}
if (ilim <= 0 && mode > 2)
{
    if (ilim < 0 || cmp(b, S = multadd(S, 5, 0)) <= 0)
    {
        /* no digits, fcvt style */
    no_digits:
        k = -1 - ndigits;
        goto ret;
    }
one_digit:
    *s++ = '1';
    k++;
    goto ret;
}
if (leftright)
{
    if (m2 > 0)
    {
        mhi = lshift(mhi, m2);
    }

    /* Compute mlo -- check for special case
		 * that d is a normalized power of 2.
		 */

    mlo = mhi;
    if (spec_case)
    {
        mhi = Balloc(mhi->k);
        Bcopy(mhi, mlo);
        mhi = lshift(mhi, Log2P);
    }

    for (i = 1;; i++)
    {
        dig = quorem(b, S) + '0';
        /* Do we yet have the shortest decimal string
			 * that will round to d?
			 */
        j = cmp(b, mlo);
        delta = diff(S, mhi);
        j1 = delta->sign ? 1 : cmp(b, delta);
        Bfree(delta);
        if (j1 == 0 && !mode && !(word1(d) & 1))
        {
            if (dig == '9')
            {
                goto round_9_up;
            }
            if (j > 0)
            {
                dig++;
            }
            *s++ = dig;
            goto ret;
        }
        if (j < 0 || j == 0 && !mode && !(word1(d) & 1))
        {
            if (j1 > 0)
            {
                b = lshift(b, 1);
                j1 = cmp(b, S);
                if ((j1 > 0 || j1 == 0 && dig & 1) && dig++ == '9')
                {
                    goto round_9_up;
                }
            }
            *s++ = dig;
            goto ret;
        }
        if (j1 > 0)
        {
            if (dig == '9')
            { /* possible if i == 1 */
            round_9_up:
                *s++ = '9';
                goto roundoff;
            }
            *s++ = dig + 1;
            goto ret;
        }
        *s++ = dig;
        if (i == ilim)
        {
            break;
        }
        b = multadd(b, 10, 0);
        if (mlo == mhi)
        {
            mlo = mhi = multadd(mhi, 10, 0);
        }
        else
        {
            mlo = multadd(mlo, 10, 0);
            mhi = multadd(mhi, 10, 0);
        }
    }
}
else
{
    for (i = 1;; i++)
    {
        *s++ = dig = quorem(b, S) + '0';
        if (i >= ilim)
        {
            break;
        }
        b = multadd(b, 10, 0);
    }
}

/* Round off last digit */

b = lshift(b, 1);
j = cmp(b, S);
if (j > 0 || j == 0 && dig & 1)
{
roundoff:
    while (*--s == '9')
    {
        if (s == s0)
        {
            k++;
            *s++ = '1';
            goto ret;
        }
    }
    ++*s++;
}
else
{
    while (*--s == '0')
        ;
    s++;
}
ret: Bfree(S);
if (mhi)
{
    if (mlo && mlo != mhi)
    {
        Bfree(mlo);
    }
    Bfree(mhi);
}
ret1: Bfree(b);
*s = 0;
*decpt = k + 1;
if (rve)
{
    *rve = s;
}
return s0;
}
