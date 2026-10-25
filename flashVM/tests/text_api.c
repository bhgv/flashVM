#include <stdio.h>
#include <stdlib.h>
#include "flashvm/utf8.h"
#include "flashvm/regex.h"


int main(void)
{
    size_t count = 0, pos = 0, n = 0, start = 0, len = 0;
    unsigned long cp = 0;
    char out[4];
    const char sample[] = "A\xD0\x96\xF0\x9F\x98\x80";
    if (!(fvm_utf8_validate(sample, 7, &count) == FVM_UTF8_OK && count == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_utf8_read(sample, 7, &pos, &cp) == FVM_UTF8_OK && cp == 65))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_utf8_read(sample, 7, &pos, &cp) == FVM_UTF8_OK && cp == 0x416))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_utf8_write(cp, out, sizeof(out), &n) == FVM_UTF8_OK && n == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_utf8_validate("\xC0\x80", 2, 0) == FVM_UTF8_INVALID))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_utf8_validate("\xED\xA0\x80", 3, 0) == FVM_UTF8_INVALID))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_utf8_offset(sample, 7, 2, &pos) == FVM_UTF8_OK && pos == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_regex_search("^ab+c$", "abbbc", 5, &start, &len) == 1 && start == 0 && len == 5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_regex_search("x.y", "zzxayzz", 7, &start, &len) == 1 && start == 2 && len == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_regex_search("^a$", "b", 1, &start, &len) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
