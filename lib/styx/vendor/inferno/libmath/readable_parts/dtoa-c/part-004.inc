/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of dtoa.c; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */

            rv /= tens[i];
        }
        if (e1 &= ~15)
        {
            e1 >>= 4;
            if (e1 >= 1 << n_bigtens)
            {
                goto undfl;
            }
            if (e1 & Scale_Bit)
            {
                scale = P;
            }
            for (j = 0; e1 > 0; j++, e1 >>= 1)
            {
                if (e1 & 1)
                {
                    rv *= tinytens[j];
                }
            }
            if (!rv)
            {
            undfl:
                rv = 0.;
                if (bd0)
                {
                    goto retfree;
                }
                goto ret;
            }
        }
    }

    /* Now the hard part -- adjusting rv to the correct value.*/

    /* Put digits into bd: true value = bd * 10^e */

    bd0 = s2b(s0, nd0, nd, y);

    for (;;)
    {
        bd = Balloc(bd0->k);
        Bcopy(bd, bd0);
        bb = d2b(rv, &bbe, &bbbits); /* rv = bb * 2^bbe */
        bs = i2b(1);

        if (e >= 0)
        {
            bb2 = bb5 = 0;
            bd2 = bd5 = e;
        }
        else
        {
            bb2 = bb5 = -e;
            bd2 = bd5 = 0;
        }
        if (bbe >= 0)
        {
            bb2 += bbe;
        }
        else
        {
            bd2 -= bbe;
        }
        bs2 = bb2;
#ifdef Sudden_Underflow
        j = P + 1 - bbbits;
#else
        i = bbe + bbbits - 1; /* logb(rv) */
        if (i < Emin)         /* denormal */
        {
            j = bbe + (P - Emin);
        }
        else
        {
            j = P + 1 - bbbits;
        }
#endif
        bb2 += j;
        bd2 += j;
        bd2 += scale;
        i = bb2 < bd2 ? bb2 : bd2;
        if (i > bs2)
        {
            i = bs2;
        }
        if (i > 0)
        {
            bb2 -= i;
            bd2 -= i;
            bs2 -= i;
        }
        if (bb5 > 0)
        {
            bs = pow5mult(bs, bb5);
            bb1 = mult(bs, bb);
            Bfree(bb);
            bb = bb1;
        }
        if (bb2 > 0)
        {
            bb = lshift(bb, bb2);
        }
        if (bd5 > 0)
        {
            bd = pow5mult(bd, bd5);
        }
        if (bd2 > 0)
        {
            bd = lshift(bd, bd2);
        }
        if (bs2 > 0)
        {
            bs = lshift(bs, bs2);
        }
        delta = diff(bb, bd);
        dsign = delta->sign;
        delta->sign = 0;
        i = cmp(delta, bs);
        if (i < 0)
        {
            /* Error is less than half an ulp -- check for
			 * special case of mantissa a power of two.
			 */
            if (dsign || word1(rv) || word0(rv) & Bndry_mask || (word0(rv) & Exp_mask) <= Exp_msk1)
            {
                if (!delta->x[0] && delta->wds == 1)
                {
                    dsign = 2;
                }
                break;
            }
            delta = lshift(delta, Log2P);
            if (cmp(delta, bs) > 0)
            {
                goto drop_down;
            }
            break;
        }
        if (i == 0)
        {
            /* exactly half-way between */
            if (dsign)
            {
                if ((word0(rv) & Bndry_mask1) == Bndry_mask1 && word1(rv) == 0xffffffff)
                {
                    /*boundary case -- increment exponent*/
                    word0(rv) = (word0(rv) & Exp_mask) + Exp_msk1;
                    word1(rv) = 0;
                    dsign = 0;
                    break;
                }
            }
            else if (!(word0(rv) & Bndry_mask) && !word1(rv))
            {
                dsign = 2;
            drop_down:
                /* boundary case -- decrement exponent */
#ifdef Sudden_Underflow
                L = word0(rv) & Exp_mask;
                if (L <= Exp_msk1)
                {
                    goto undfl;
                }
                L -= Exp_msk1;
#else
                L = (word0(rv) & Exp_mask) - Exp_msk1;
#endif
                word0(rv) = L | Bndry_mask1;
                word1(rv) = 0xffffffff;
                break;
            }
            if (!(word1(rv) & LSB))
            {
                break;
            }
            if (dsign)
            {
                rv += ulp(rv);
            }
            else
            {
                rv -= ulp(rv);
#ifndef Sudden_Underflow
                if (!rv)
                {
                    goto undfl;
                }
#endif
            }
            dsign = 1 - dsign;
            break;
        }
        if ((aadj = ratio(delta, bs)) <= 2.)
        {
            if (dsign)
            {
                aadj = aadj1 = 1.;
            }
            else if (word1(rv) || word0(rv) & Bndry_mask)
            {
#ifndef Sudden_Underflow
                if (word1(rv) == Tiny1 && !word0(rv))
                {
                    goto undfl;
                }
#endif
                aadj = 1.;
                aadj1 = -1.;
            }
            else
            {
                /* special case -- power of FLT_RADIX to be */
                /* rounded down... */

                if (aadj < 2. / FLT_RADIX)
                {
                    aadj = 1. / FLT_RADIX;
                }
                else
                {
                    aadj *= 0.5;
                }
                aadj1 = -aadj;
            }
        }
        else
        {
            aadj *= 0.5;
            aadj1 = dsign ? aadj : -aadj;
            if (FLT_ROUNDS == 0)
            {
                aadj1 += 0.5;
            }
        }
        y = word0(rv) & Exp_mask;

        /* Check for overflow */

        if (y == Exp_msk1 * (DBL_MAX_EXP + Bias - 1))
        {
            rv0 = rv;
            word0(rv) -= P * Exp_msk1;
            adj = aadj1 * ulp(rv);
            rv += adj;
            if ((word0(rv) & Exp_mask) >= Exp_msk1 * (DBL_MAX_EXP + Bias - P))
            {
                if (word0(rv0) == Big0 && word1(rv0) == Big1)
                {
                    goto ovfl;
                }
                word0(rv) = Big0;
                word1(rv) = Big1;
                goto cont;
            }
            else
            {
                word0(rv) += P * Exp_msk1;
            }
        }
        else
        {
#ifdef Sudden_Underflow
            if ((word0(rv) & Exp_mask) <= P * Exp_msk1)
            {
                rv0 = rv;
                word0(rv) += P * Exp_msk1;
                adj = aadj1 * ulp(rv);
                rv += adj;
                if ((word0(rv) & Exp_mask) <= P * Exp_msk1)
                {
                    if (word0(rv0) == Tiny0 && word1(rv0) == Tiny1)
                    {
                        goto undfl;
                    }
                    word0(rv) = Tiny0;
                    word1(rv) = Tiny1;
                    goto cont;
                }
                else
                {
                    word0(rv) -= P * Exp_msk1;
                }
            }
            else
            {
                adj = aadj1 * ulp(rv);
                rv += adj;
            }
#else
            /* Compute adj so that the IEEE rounding rules will
			 * correctly round rv + adj in some half-way cases.
			 * If rv * ulp(rv) is denormalized (i.e.,
			 * y <= (P-1)*Exp_msk1), we must adjust aadj to avoid
			 * trouble from bits lost to denormalization;
			 * example: 1.2e-307 .
			 */
            if (y <= (P - 1) * Exp_msk1 && aadj >= 1.)
            {
                aadj1 = (double)(int)(aadj + 0.5);
                if (!dsign)
                {
                    aadj1 = -aadj1;
                }
            }
            adj = aadj1 * ulp(rv);
            rv += adj;
#endif
        }
        z = word0(rv) & Exp_mask;
        if (!scale)
        {
            if (y == z)
            {
                /* Can we stop now? */
                L = aadj;
                aadj -= L;
                /* The tolerances below are conservative. */
                if (dsign || word1(rv) || word0(rv) & Bndry_mask)
                {
                    if (aadj < .4999999 || aadj > .5000001)
                    {
                        break;
                    }
                }
                else if (aadj < .4999999 / FLT_RADIX)
                {
                    break;
                }
            }
        }
    cont:
        Bfree(bb);
        Bfree(bd);
        Bfree(bs);
        Bfree(delta);
    }
    if (scale)
    {
        if ((word0(rv) & Exp_mask) <= P * Exp_msk1 && word1(rv) & 1 && dsign != 2)
        {
            if (dsign)
            {
                rv += ulp(rv);
            }
            else
            {
                word1(rv) &= ~1;
            }
        }
        word0(rv0) = Exp_1 - P * Exp_msk1;
        word1(rv0) = 0;
        rv *= rv0;
    }
retfree:
    Bfree(bb);
    Bfree(bd);
    Bfree(bs);
    Bfree(bd0);
    Bfree(delta);
ret:
    if (se)
    {
        *se = (char *)s;
    }
    return sign ? -rv : rv;
}

/* [standalone-contract:quorem]
 * EN: quorem: original decimal conversion with checked integer limbs and synchronized public entry points.
 * RU: quorem: исходное decimal-преобразование с проверенными limbs и синхронизированными публичными входами.
 * b (Bigint *b): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * S (Bigint *S): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: int; direct result constants in this body: 0. Forwarded results preserve the called API's status.
 * Возврат: int; прямые константы результата тела: 0. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static int quorem(Bigint *b, Bigint *S)
{
    int n;
    int borrow, y;
    unsigned int carry, q, ys;
    unsigned int *bx, *bxe, *sx, *sxe;
    int z;
    unsigned int si, zs;

    n = S->wds;
    if (b->wds < n)
    {
        return 0;
    }
    sx = S->x;
    sxe = sx + --n;
    bx = b->x;
    bxe = bx + n;
    q = *bxe / (*sxe + 1); /* ensure q <= true quotient */
    if (q)
    {
        borrow = 0;
        carry = 0;
        do
        {
            si = *sx++;
            ys = (si & 0xffff) * q + carry;
            zs = (si >> 16) * q + (ys >> 16);
            carry = zs >> 16;
            y = (*bx & 0xffff) - (ys & 0xffff) + borrow;
            borrow = y >> 16;
            Sign_Extend(borrow, y);
            z = (*bx >> 16) - (zs & 0xffff) + borrow;
            borrow = z >> 16;
            Sign_Extend(borrow, z);
            Storeinc(bx, z, y);
        } while (sx <= sxe);
        if (!*bxe)
        {
            bx = b->x;
            while (--bxe > bx && !*bxe)
            {
                --n;
            }
            b->wds = n;
        }
    }
    if (cmp(b, S) >= 0)
    {
