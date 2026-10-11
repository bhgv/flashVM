/* C89 example: count Unicode scalars, then search an ASCII pattern. */
#include "flashvm/utf8.h"
#include "flashvm/regex.h"
#include <stdio.h>
#include <string.h>
int main(void) {
    const char *text = "Hello, \xD0\xBC\xD0\xB8\xD1\x80!";
    size_t scalars = 0, start = 0, length = 0;
    if (fvm_utf8_validate(text, strlen(text), &scalars) != FVM_UTF8_OK)
        return 1;
    printf("Unicode scalars: %lu\n", (unsigned long)scalars);
    if (fvm_regex_search("Hello", text, strlen(text), &start, &length) == 1)
        printf("Match: byte %lu, length %lu\n", (unsigned long)start,
               (unsigned long)length);
    return 0;
}
