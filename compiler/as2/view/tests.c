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
                         "panel.x = bus.send(1, 2, 3, 4, 5, 6, 7, 8, 9);",
                         "panel.x = 3; @",
                         "panel.x = (3;",
                         "response = bus.send(ref values[]);",
                         "values[] = 1;",
                         "function onEnterFrame(arg, arg) { ready = arg; }",
                         "function onEnterFrame(a,b,c,d,e,f,g,h,i) { return a; }",
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
    const char *indexed = "captured = bus.send(ref rows[bus.recv()]); rows[key] = bus.recv(); "
                          "read = rows[key]; named = rows.count;";
    if (!as2_view_compile(indexed, strlen(indexed), output, sizeof(output) - 1, &length, &diagnostic))
    {
        fprintf(stderr, "computed AS properties or ref target rejected\n");
        return 1;
    }
    output[length] = 0;
    const char *owner = strstr((const char *)output, "as.global_get \"rows\"");
    const char *key = owner ? strstr(owner, ", \"recv\"\n") : NULL;
    if (!key || !strstr(key, ", \"ref\",") || !strstr(key, "as.member_put ") ||
        !strstr(key, "as.member_get ") || !strstr(key, "const.string \"count\""))
    {
        fprintf(stderr, "computed owner/key order or member bytecode lowering changed\n");
        return 1;
    }
    const char *callback = "function invoke(op, buf, len) { buf[0] = len; return buf[0]; }";
    if (!as2_view_compile(callback, strlen(callback), output, sizeof(output) - 1, &length, &diagnostic))
    {
        fprintf(stderr, "parameterized AS2 callback rejected\n");
        return 1;
    }
    output[length] = 0;
    if (!strstr((const char *)output, "source_entry avm1_method @invoke @op @buf @len\n") ||
        !strstr((const char *)output, "return %v"))
    {
        fprintf(stderr, "callback parameters or result lost\n");
        return 1;
    }
    const char *method = "function drawItem() { value = target.draw(1, bus.receive(), 3); }";
    if (!as2_view_compile(method, strlen(method), output, sizeof(output) - 1, &length, &diagnostic))
    {
        fprintf(stderr, "valid named root method rejected\n");
        return 1;
    }
    output[length] = 0;
    if (!strstr((const char *)output, "source_entry avm1_method @drawItem\n") ||
        !strstr((const char *)output, "as.member_call_args") || !strstr((const char *)output, "as.args = as.arg"))
    {
        fprintf(stderr, "method name or argument list was lost\n");
        return 1;
    }
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
    const char *detached = "asinc // worker entry\nfunction compute(data, delta) { data[0]=data[0]+delta; return data[0]; }";
    if (!as2_view_compile(detached, strlen(detached), output, sizeof(output) - 1, &length, &diagnostic))
    {
        fprintf(stderr, "detached asinc rejected\n");
        return 1;
    }
    output[length] = 0;
    if (!strstr((const char *)output, "source_entry avm1_asinc @compute @data @delta\n"))
    {
        fprintf(stderr, "asinc dispatch metadata lost\n");
        return 1;
    }
    const char *unsupported[] = {
        "asinc function work(value,value) { return value; }",
        "asinc function work() { return outer; }",
        "asinc function work() { counter=42; }",
        "asinc function work() { var alias=ref outer; return alias; }",
        "asinc { var x=41; }",
    };
    for (size_t i = 0; i < sizeof(unsupported) / sizeof(*unsupported); ++i)
    {
        output[0] = 0xa5;
        if (as2_view_compile(unsupported[i], strlen(unsupported[i]), output, sizeof(output), &length, &diagnostic) ||
            length || output[0] != 0xa5)
        {
            fprintf(stderr, "unsupported asinc replaced the artifact\n");
            return 1;
        }
    }
    const char *ordinary = "var asinc = 42;";
    if (!as2_view_compile(ordinary, strlen(ordinary), output, sizeof(output), &length, &diagnostic))
    {
        fprintf(stderr, "ordinary asinc binding rejected\n");
        return 1;
    }
    puts("PASS AS2 display library: methods, literals, failure recovery and atomic output");
    return 0;
}
