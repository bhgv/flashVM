/* Exercise the public compiler library, not a mirror of its FIR printer. */
#include <stdio.h>
#include <string.h>
#include "view.h"

/* Check repeated recovery and atomic output after malformed/profile/capacity failures. */
int main(void)
{
    const char *good = "panel.text = text; panel._x = width - 16; "
                       "sent = bus.send(bus.receive() + \"Я\\n\\\"\"); // EOF comment";
    const char *handler = "function onEnterFrame() { if (!(position - 1)) { received = bus.receive(); "
                          "if (received) { sent = bus.send(received); } } else { ready = true; } }";
    const char *bad[] = {"panel.x = 2147483648;",
                         "panel.x = value();",
                         "panel.x = bus.send(1, 2);",
                         "panel.x = 3; @",
                         "panel.x = (3;",
                         "function unsupported() { ready = true; }",
                         "function onEnterFrame(arg) { ready = arg; }",
                         "if (ready) ready = false;",
                         "panel.x = 3; // \xc0\xaf",
                         "panel.x = 3; // \xe2\x80\xa8" "panel.x = 4;",
                         "",
                         "panel.x = 3; class A {}"};
    unsigned char output[4097];
    unsigned char expected[4096];
    size_t length;
    size_t original;
    size_t index;
    int cycle;
    As2ViewDiagnostic diagnostic;
    if (!as2_view_compile(handler, strlen(handler), output, sizeof(output) - 1, &length, &diagnostic))
    {
        fprintf(stderr, "valid timeline handler rejected\n");
        return 1;
    }
    output[length] = 0;
    if (!strstr((const char *)output, "source_entry avm1_frame\n") ||
        !strstr((const char *)output, "as.to_boolean") || !strstr((const char *)output, "bool.not") ||
        !strstr((const char *)output, "^merge1:"))
    {
        fprintf(stderr, "frame handler metadata or nested control flow was lost\n");
        return 1;
    }
    if (!as2_view_compile(good, strlen(good), output, sizeof(output) - 1, &original, &diagnostic))
    {
        fprintf(stderr, "valid display source rejected\n");
        return 1;
    }
    memcpy(expected, output, original);
    output[original] = 0;
    if (strstr((const char *)output, "source_entry") || !strstr((const char *)output, ", \"receive\"\n") ||
        !strstr((const char *)output, ", \"send\", %") ||
        !strstr((const char *)output, "const.string \"Я\\n\\\"\""))
    {
        fprintf(stderr, "nested method or string was lowered incorrectly\n");
        return 1;
    }
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
    puts("PASS AS2 display library: methods, literals, failure recovery and atomic output");
    return 0;
}
