#ifndef FVM_MINI_REGEX_H
#define FVM_MINI_REGEX_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Small portable ASCII/byte regex subset; no external library. */
int fvm_mini_regex_search(const char *pattern,const char *text,size_t text_length,size_t *start,size_t *length);
#ifdef __cplusplus
}
#endif
#endif
