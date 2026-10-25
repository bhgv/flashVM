/* FlashVM: Public embedding API declarations: regex.
 * File: include/flashvm/regex.h.
 */

#ifndef FLASHVM_REGEX_H
/* Define FLASHVM REGEX H constant or compilation switch. */
#define FLASHVM_REGEX_H
#include <stddef.h>
#ifdef __cplusplus
extern "C"
{
#endif
/* 1 match, 0 no match, -1 invalid arguments, -2 budget exhausted. */


/* Implement regex search for this subsystem. */
int fvm_regex_search(const char *pattern, const char *text, size_t bytes, size_t *start,
                     size_t *match_bytes);
#ifdef __cplusplus
}
#endif
#endif
