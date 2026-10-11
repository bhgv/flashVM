#ifndef FLASHVM_REGEX_H
#define FLASHVM_REGEX_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* 1 match, 0 no match, -1 invalid arguments, -2 budget exhausted. */
int fvm_regex_search(const char *pattern,const char *text,size_t bytes,size_t *start,size_t *match_bytes);
#ifdef __cplusplus
}
#endif
#endif
