/* FlashVM: C embedding API: regex.
 * File: src/capi/regex.c.
 */

#include "flashvm/regex.h"
#include "mini_regex.h"
/* Compute and return regex search. */
int fvm_regex_search(const char *p, const char *s, size_t n, size_t *start, size_t *len)
{
    return fvm_mini_regex_search(p, s, n, start, len);
}
