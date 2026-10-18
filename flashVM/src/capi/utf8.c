/* FlashVM: C embedding API: utf8.
 * File: src/capi/utf8.c.
 */

#include "flashvm/utf8.h"
/* Read utf8. */
FVM_UTF8_Status fvm_utf8_read(const char *s, size_t n, size_t *pos, unsigned long *cp)
{

    /* Initialization / Инициализация локального состояния. */
    size_t i;
    unsigned long c;
    unsigned int k, j;
    unsigned char b;
    if (!s || !pos || !cp || *pos >= n)
        return FVM_UTF8_INVALID;

    /* Computation / Вычисление и обновление состояния. */
    i = *pos;
    b = (unsigned char)s[i];
    if (b < 0x80)
    {
        c = b;
        k = 1;
    }
    else if (b >= 0xC2 && b <= 0xDF)
    {
        c = b & 31;
        k = 2;
    }
    else if (b >= 0xE0 && b <= 0xEF)
    {
        c = b & 15;
        k = 3;
    }
    else if (b >= 0xF0 && b <= 0xF4)
    {
        c = b & 7;
        k = 4;
    }
    else
        return FVM_UTF8_INVALID;
    if (k > n - i)
        return FVM_UTF8_INVALID;
    for (j = 1; j < k; j++)
    {
        b = (unsigned char)s[i + j];
        if ((b & 0xC0) != 0x80)
            return FVM_UTF8_INVALID;
        c = (c << 6) | (b & 63);
    }
    if ((k == 2 && c < 0x80) || (k == 3 && c < 0x800) || (k == 4 && c < 0x10000) || c > 0x10FFFF ||
        (c >= 0xD800 && c <= 0xDFFF))
        return FVM_UTF8_INVALID;

    /* Computation / Вычисление и обновление состояния. */
    *pos = i + k;
    *cp = c;
    return FVM_UTF8_OK;
}

/* Validate utf8. */
FVM_UTF8_Status fvm_utf8_validate(const char *s, size_t n, size_t *count)
{

    /* Initialization / Инициализация локального состояния. */
    size_t p = 0, c = 0;
    unsigned long cp;
    if (!s && n)
        return FVM_UTF8_INVALID;
    while (p < n)
    {
        if (fvm_utf8_read(s, n, &p, &cp) != FVM_UTF8_OK)
            return FVM_UTF8_INVALID;
        c++;
    }
    if (count)
        *count = c;
    return FVM_UTF8_OK;
}
/* Write utf8. */
FVM_UTF8_Status fvm_utf8_write(unsigned long cp, char *out, size_t cap, size_t *len)
{
    unsigned int k, j;
    if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
        return FVM_UTF8_RANGE;
    k = cp < 0x80 ? 1 : cp < 0x800 ? 2 : cp < 0x10000 ? 3 : 4;
    if (!out || cap < k)
        return FVM_UTF8_NOSPACE;
    for (j = k - 1; j > 0; j--)
    {
        out[j] = (char)(0x80 | (cp & 63));
        cp >>= 6;
    }
    out[0] = (char)(cp | (k == 1 ? 0 : k == 2 ? 0xC0 : k == 3 ? 0xE0 : 0xF0));
    if (len)
        *len = k;
    return FVM_UTF8_OK;
}

/* Compute and return utf8 offset. */
FVM_UTF8_Status fvm_utf8_offset(const char *s, size_t n, size_t index, size_t *out)
{

    /* Initialization / Инициализация локального состояния. */
    size_t p = 0, j;
    unsigned long cp;
    if (!out || (!s && n))
        return FVM_UTF8_INVALID;
    for (j = 0; j < index; j++)
        if (fvm_utf8_read(s, n, &p, &cp) != FVM_UTF8_OK)
            return FVM_UTF8_RANGE;
    *out = p;
    return FVM_UTF8_OK;
}
