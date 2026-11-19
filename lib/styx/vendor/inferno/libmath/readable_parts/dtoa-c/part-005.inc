/* Copyright (c) 2026 bhgv — readability annotations only; original notices remain in the accompanying source.
 * EN: Sequential implementation section of dtoa.c; inherits the facade's C scope and original declarations.
 * RU: Последовательная часть реализации; наследует C scope и исходные declarations facade. */
        q++;

        borrow = 0;

        carry = 0;
        bx = b->x;
        sx = S->x;
        do
        {
            si = *sx++;
            ys = (si & 0xffff) + carry;
            zs = (si >> 16) + (ys >> 16);
            carry = zs >> 16;
            y = (*bx & 0xffff) - (ys & 0xffff) + borrow;
            borrow = y >> 16;
            Sign_Extend(borrow, y);
            z = (*bx >> 16) - (zs & 0xffff) + borrow;
            borrow = z >> 16;
            Sign_Extend(borrow, z);
            Storeinc(bx, z, y);
        } while (sx <= sxe);
        bx = b->x;
        bxe = bx + n;
        if (!*bxe)
        {
            while (--bxe > bx && !*bxe)
            {
                --n;
            }
            b->wds = n;
        }
    }
    return q;
}

/* [standalone-contract:rv_alloc]
 * EN: rv_alloc: original decimal conversion with checked integer limbs and synchronized public entry points.
 * RU: rv_alloc: исходное decimal-преобразование с проверенными limbs и синхронизированными публичными входами.
 * i (int i): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static char *rv_alloc(int i)
{
    int j, k, *r;

    j = sizeof(unsigned int);
    for (k = 0; sizeof(Bigint) - sizeof(unsigned int) - sizeof(int) + j <= i; j <<= 1)
    {
        k++;
    }
    r = (int *)Balloc(k);
    *r = k;
    return (char *)(r + 1);
}

/* [standalone-contract:nrv_alloc]
 * EN: nrv_alloc: original decimal conversion with checked integer limbs and synchronized public entry points.
 * RU: nrv_alloc: исходное decimal-преобразование с проверенными limbs и синхронизированными публичными входами.
 * s (char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * rve (char **rve): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * n (int n): Extent/count used by the selected conversion; byte/rune/record units follow its signature and description.
 * RU: Длина/число выбранного преобразования; единицы bytes/runes/records заданы сигнатурой и описанием.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
static char *nrv_alloc(char *s, char **rve, int n)
{
    char *rv, *t;

    t = rv = rv_alloc(n);
    while (*t = *s++)
    {
        t++;
    }
    if (rve)
    {
        *rve = t;
    }
    return rv;
}

/* freedtoa(s) must be used to free values s returned by dtoa
 * when MULTIPLE_THREADS is #defined.  It should be used in all cases,
 * but for consistency with earlier versions of dtoa, it is optional
 * when MULTIPLE_THREADS is not defined.
 */


/* [standalone-contract:limbo_decimal_free_unlocked]
 * EN: limbo_decimal_free_unlocked: original decimal conversion with checked integer limbs and synchronized public entry points.
 * RU: limbo_decimal_free_unlocked: исходное decimal-преобразование с проверенными limbs и синхронизированными публичными входами.
 * s (char *s): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void limbo_decimal_free_unlocked(char *s)
{
    Bigint *b = (Bigint *)((int *)s - 1);
    b->maxwds = 1 << (b->k = *(int *)b);
    Bfree(b);
}

/* dtoa for IEEE arithmetic (dmg): convert double to ASCII string.
 *
 * Inspired by "How to Print Floating-Point Numbers Accurately" by
 * Guy L. Steele, Jr. and Jon L. White [Proc. ACM SIGPLAN '90, pp. 92-101].
 *
 * Modifications:
 *	1. Rather than iterating, we use a simple numeric overestimate
 *	   to determine k = floor(log10(d)).  We scale relevant
 *	   quantities using O(log2(k)) rather than O(k) multiplications.
 *	2. For some modes > 2 (corresponding to ecvt and fcvt), we don't
 *	   try to generate digits strictly left to right.  Instead, we
 *	   compute with fewer bits and propagate the carry if necessary
 *	   when rounding the final digit up.  This is often faster.
 *	3. Under the assumption that input will be rounded nearest,
 *	   mode 0 renders 1e23 as 1e23 rather than 9.999999999999999e22.
 *	   That is, we allow equality in stopping tests when the
 *	   round-nearest rule will give the same floating-point value
 *	   as would satisfaction of the stopping test with strict
 *	   inequality.
 *	4. We remove common factors of powers of 2 from relevant
 *	   quantities.
 *	5. When converting floating-point integers less than 1e16,
 *	   we use floating-point arithmetic rather than resorting
 *	   to multiple-precision integers.
 *	6. When asked to produce fewer than 15 digits, we first try
 *	   to get by with floating-point arithmetic; we resort to
 *	   multiple-precision integer arithmetic only if we cannot
 *	   guarantee that the floating-point calculation has given
 *	   the correctly rounded result.  For k requested digits and
 *	   "uniformly" distributed input, the probability is
 *	   something like 10^(k-15) that we must resort to the long
 *	   calculation.
 */


/* [standalone-contract:limbo_decimal_dtoa_unlocked]
 * EN: limbo_decimal_dtoa_unlocked: original decimal conversion with checked integer limbs and synchronized public entry points.
 * RU: limbo_decimal_dtoa_unlocked: исходное decimal-преобразование с проверенными limbs и синхронизированными публичными входами.
 * d (double d): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * mode (int mode): Requested operation/permission mode; use the documented protocol constants, not a pathname suffix.
 * RU: Режим операции/доступа; использовать протокольные константы, не суффикс имени.
 * ndigits (int ndigits): Scalar operand of this operation; protocol/algorithm ranges are described below.
 * RU: Скалярный операнд операции; диапазон протокола/алгоритма описан ниже.
 * decpt (int *decpt): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * sign (int *sign): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * rve (char **rve): Referenced value of the declared type; lifetime and input/output effects remain those of the implementation.
 * RU: Ссылка на значение объявленного типа; время жизни и input/output-эффекты соответствуют реализации.
 * Return: char *; direct result constants in this body: see Description and the implementation. Forwarded results preserve the called API's status.
 * Возврат: char *; прямые константы результата тела: see Description and the implementation. Пересылаемый результат сохраняет статус вызываемого API.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
char *limbo_decimal_dtoa_unlocked(double d, int mode, int ndigits, int *decpt, int *sign, char **rve)
{
    /*	Arguments ndigits, decpt, sign are similar to those
	of ecvt and fcvt; trailing zeros are suppressed from
	the returned string.  If not null, *rve is set to point
	to the end of the return value.  If d is +-Infinity or NaN,
	then *decpt is set to 9999.

	mode:
		0 ==> shortest string that yields d when read in
			and rounded to nearest.
		1 ==> like 0, but with Steele & White stopping rule;
			e.g. with IEEE P754 arithmetic , mode 0 gives
			1e23 whereas mode 1 gives 9.999999999999999e22.
		2 ==> max(1,ndigits) significant digits.  This gives a
			return value similar to that of ecvt, except
			that trailing zeros are suppressed.
		3 ==> through ndigits past the decimal point.  This
			gives a return value similar to that from fcvt,
			except that trailing zeros are suppressed, and
			ndigits can be negative.
		4-9 should give the same return values as 2-3, i.e.,
			4 <= mode <= 9 ==> same return as mode
			2 + (mode & 1).  These modes are mainly for
			debugging; often they run slower but sometimes
			faster than modes 2-3.
		4,5,8,9 ==> left-to-right digit generation.
		6-9 ==> don't try fast floating-point estimate
			(if applicable).

		Values of mode other than 0-9 are treated as mode 0.

		Sufficient space is allocated to the return value
		to hold the suppressed trailing zeros.
	*/

    int bbits, b2, b5, be, dig, i, ieps, ilim, ilim0, ilim1, j, j1, k, k0, k_check, leftright, m2, m5, s2, s5,
        spec_case, try_quick;
    int L;
#ifndef Sudden_Underflow
    int denorm;
    unsigned int x;
#endif
    Bigint *b, *b1, *delta, *mlo, *mhi, *S;
    double d2, ds, eps;
    char *s, *s0;

    if (word0(d) & Sign_bit)
    {
        /* set sign for everything, including 0's and NaNs */
        *sign = 1;
        word0(d) &= ~Sign_bit; /* clear sign bit */
    }
    else
    {
        *sign = 0;
    }

    if ((word0(d) & Exp_mask) == Exp_mask)
    {
        /* Infinity or NaN */
        *decpt = 9999;
        if (!word1(d) && !(word0(d) & 0xfffff))
        {
            return nrv_alloc("Infinity", rve, 8);
        }
        return nrv_alloc("NaN", rve, 3);
    }
    if (!d)
    {
        *decpt = 1;
        return nrv_alloc("0", rve, 1);
    }

    b = d2b(d, &be, &bbits);
#ifdef Sudden_Underflow
    i = (int)(word0(d) >> Exp_shift1 & (Exp_mask >> Exp_shift1));
#else
    if (i = (int)(word0(d) >> Exp_shift1 & (Exp_mask >> Exp_shift1)))
    {
#endif
    word0(d2) = (word0(d) & Frac_mask1) | Exp_11;
    word1(d2) = word1(d);

    /* log(x)	~=~ log(1.5) + (x-1.5)/1.5
		 * log10(x)	 =  log(x) / log(10)
		 *		~=~ log(1.5)/log(10) + (x-1.5)/(1.5*log(10))
		 * log10(d) = (i-Bias)*log(2)/log(10) + log10(d2)
		 *
		 * This suggests computing an approximation k to log10(d) by
		 *
		 * k = (i - Bias)*0.301029995663981
		 *	+ ( (d2-1.5)*0.289529654602168 + 0.176091259055681 );
		 *
		 * We want k to be too large rather than too small.
		 * The error in the first-order Taylor series approximation
		 * is in our favor, so we just round up the constant enough
		 * to compensate for any error in the multiplication of
		 * (i - Bias) by 0.301029995663981; since |i - Bias| <= 1077,
		 * and 1077 * 0.30103 * 2^-52 ~=~ 7.2e-14,
		 * adding 1e-13 to the constant term more than suffices.
		 * Hence we adjust the constant term to 0.1760912590558.
		 * (We could get a more accurate k by invoking log10,
		 *  but this is probably not worthwhile.)
		 */

    i -= Bias;
#ifndef Sudden_Underflow
    denorm = 0;
}
else
{
    /* d is denormalized */

    i = bbits + be + (Bias + (P - 1) - 1);
    x = i > 32 ? word0(d) << 64 - i | word1(d) >> i - 32 : word1(d) << 32 - i;
    d2 = x;
    word0(d2) -= 31 * Exp_msk1; /* adjust exponent */
    i -= (Bias + (P - 1) - 1) + 1;
    denorm = 1;
}
#endif
ds = (d2 - 1.5) * 0.289529654602168 + 0.1760912590558 + i * 0.301029995663981;
k = (int)ds;
if (ds < 0. && ds != k)
{
    k--; /* want k = floor(ds) */
}
k_check = 1;
if (k >= 0 && k <= Ten_pmax)
{
    if (d < tens[k])
    {
        k--;
    }
    k_check = 0;
}
j = bbits - i - 1;
if (j >= 0)
{
    b2 = 0;
    s2 = j;
}
else
{
    b2 = -j;
    s2 = 0;
}
if (k >= 0)
{
    b5 = 0;
    s5 = k;
    s2 += k;
}
else
{
    b2 -= k;
    b5 = -k;
    s5 = 0;
}
if (mode < 0 || mode > 9)
{
    mode = 0;
}
try_quick = 1;
if (mode > 5)
{
    mode -= 4;
    try_quick = 0;
}
leftright = 1;

switch (mode)
{
case 0:
case 1:
    ilim = ilim1 = -1;
    i = 18;
    ndigits = 0;
    break;
case 2:
    leftright = 0;
    /* no break */
case 4:
    if (ndigits <= 0)
    {
        ndigits = 1;
    }
    ilim = ilim1 = i = ndigits;
    break;
case 3:
    leftright = 0;
    /* no break */
case 5:
    i = ndigits + k + 1;
    ilim = i;
    ilim1 = i - 1;
    if (i <= 0)
    {
        i = 1;
    }
}

s = s0 = rv_alloc(i);

if (ilim >= 0 && ilim <= Quick_max && try_quick)
{

    /* Try to get by with floating-point arithmetic. */

    i = 0;
    d2 = d;
    k0 = k;
    ilim0 = ilim;
    ieps = 2; /* conservative */
    if (k > 0)
    {
        ds = tens[k & 0xf];
        j = k >> 4;
        if (j & Bletch)
        {
            /* prevent overflows */
            j &= Bletch - 1;
            d /= bigtens[n_bigtens - 1];
            ieps++;
        }
        for (; j; j >>= 1, i++)
        {
            if (j & 1)
            {
                ieps++;
                ds *= bigtens[i];
            }
        }
        d /= ds;
    }
    else if (j1 = -k)
    {
        d *= tens[j1 & 0xf];
        for (j = j1 >> 4; j; j >>= 1, i++)
        {
            if (j & 1)
            {
                ieps++;
                d *= bigtens[i];
            }
        }
    }
    if (k_check && d < 1. && ilim > 0)
    {
        if (ilim1 <= 0)
        {
            goto fast_failed;
        }
        ilim = ilim1;
        k--;
        d *= 10.;
        ieps++;
    }
    eps = ieps * d + 7.;
    word0(eps) -= (P - 1) * Exp_msk1;
    if (ilim == 0)
