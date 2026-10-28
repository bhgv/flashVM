/* Exercise the public compiler library, not a mirror of its FIR printer. */
#include <stdio.h>
#include <string.h>
#include "view.h"

/* Check repeated recovery and atomic output after malformed/profile/capacity failures. */
int main(void)
{
    const char *good = "panel.text = text; panel._x = width - 16; // EOF comment";
    const char *bad[] = {"panel.x = 2147483648;",
                         "panel.x = value();",
                         "panel.x = 3; @",
                         "panel.x = (3;",
                         "",
                         "panel.x = 3; class A {}"};
    unsigned char output[4096];
    unsigned char expected[4096];
    size_t length;
    size_t original;
    size_t index;
    int cycle;
    As2ViewDiagnostic diagnostic;
    if (!as2_view_compile(good, strlen(good), output, sizeof(output), &original, &diagnostic))
    {
        fprintf(stderr, "valid display source rejected\n");
        return 1;
    }
    memcpy(expected, output, original);
    for (cycle = 0; cycle < 100; cycle++)
    {
        for (index = 0; index < sizeof(bad) / sizeof(bad[0]); index++)
        {
            memset(output, 0xa5, sizeof(output));
            if (as2_view_compile(bad[index], strlen(bad[index]), output, sizeof(output), &length, &diagnostic) ||
                length)
            {
                fprintf(stderr, "invalid source accepted: %s\n", bad[index]);
                return 1;
            }
            for (length = 0; length < sizeof(output); length++)
            {
                if (output[length] != 0xa5)
                {
                    fprintf(stderr, "partial compiler output escaped\n");
                    return 1;
                }
            }
            if (!as2_view_compile(good, strlen(good), output, sizeof(output), &length, &diagnostic) ||
                length != original || memcmp(output, expected, original))
            {
                fprintf(stderr, "compiler recovery changed the next valid compilation\n");
                return 1;
            }
        }
    }
    memset(output, 0xa5, sizeof(output));
    if (as2_view_compile(good, strlen(good), output, 1, &length, &diagnostic) || length || output[0] != 0xa5)
    {
        fprintf(stderr, "capacity failure was not atomic\n");
        return 1;
    }
    puts("PASS AS2 display library: shared frontend, 600 failure/recovery cycles and atomic output");
    return 0;
}
