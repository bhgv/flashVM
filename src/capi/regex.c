#include "flashvm/regex.h"
#include "mini_regex.h"
int fvm_regex_search(const char *p,const char *s,size_t n,size_t *start,size_t *len){return fvm_mini_regex_search(p,s,n,start,len);}
