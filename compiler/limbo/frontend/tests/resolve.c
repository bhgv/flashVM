/* Check lexical alias semantics against owned Coco/R nodes, without another parser. */
#include "resolve.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


/* Parse copied source, resolve x in module M (or file scope), and check exact status.
 * text/kind are borrowed. Every catalog is destroyed; failure outputs must be retained. */
static int check(const char *text, const char *kind, int code)
{
    LimboInterface *owner = NULL;
    LimboDiagnostic diagnostic;
    size_t module = SIZE_MAX;
    size_t declaration;
    size_t type;
    size_t output = SIZE_MAX - 1;
    int accepted;
    if (!limbo_interface_parse(text, strlen(text), &owner, &diagnostic))
    {
        fprintf(stderr, "alias fixture parse failed: %s\n", diagnostic.message);
        return 0;
    }
    limbo_interface_find(owner, SIZE_MAX, "M", &module);
    {
        size_t nested;
        if (limbo_interface_find(owner, module, "N", &nested))
        {
            module = nested;
        }
    }
    if (!limbo_interface_find(owner, module, "x", &declaration) ||
        !limbo_interface_declared_type(owner, declaration, &type))
    {
        limbo_interface_destroy(owner);
        return 0;
    }
    accepted = limbo_interface_resolve_type(owner, module, type, &output, &diagnostic);
    if ((accepted != (code == 0)) || diagnostic.code != code ||
        (accepted && strcmp(limbo_interface_type_kind(owner, output), kind)) ||
        (!accepted &&
         (output != SIZE_MAX - 1 || diagnostic.offset < 0 || diagnostic.line < 1 || diagnostic.column < 1)))
    {
        fprintf(stderr, "alias status mismatch: expected %d, got %d: %s\n", code, diagnostic.code, diagnostic.message);
        limbo_interface_destroy(owner);
        return 0;
    }
    limbo_interface_destroy(owner);
    return 1;
}


/* Build a bounded chain at and above the public work limit; no recursion or inferred syntax. */
static int depth(unsigned int count, int expected)
{
    char source[32768];
    size_t used = 0;
    unsigned int index;
    for (index = 0; index < count; index++)
    {
        int written;
        if (index + 1 == count)
        {
            written = snprintf(source + used, sizeof(source) - used, "A%u: type big;\n", index);
        }
        else
        {
            written = snprintf(source + used, sizeof(source) - used, "A%u: type A%u;\n", index, index + 1);
        }
        if (written < 0 || (size_t)written >= sizeof(source) - used)
        {
            return 0;
        }
        used += (size_t)written;
    }
    if (sizeof(source) - used < sizeof("x: A0;"))
    {
        return 0;
    }
    strcpy(source + used, "x: A0;");
    return check(source, "big", expected);
}


/* Diagnostics identify the exact UTF-8 byte position; invalid queries preserve output. */
static int diagnostics(void)
{
    const char *source = "M: module {\n x: Missing;\n};";
    LimboInterface *owner = NULL;
    LimboDiagnostic diagnostic;
    size_t module;
    size_t declaration;
    size_t type;
    size_t output = SIZE_MAX - 1;
    int ok;
    if (!limbo_interface_parse(source, strlen(source), &owner, &diagnostic) ||
        !limbo_interface_find(owner, SIZE_MAX, "M", &module) ||
        !limbo_interface_find(owner, module, "x", &declaration) ||
        !limbo_interface_declared_type(owner, declaration, &type))
    {
        limbo_interface_destroy(owner);
        return 0;
    }
    ok = !limbo_interface_resolve_type(owner, module, type, &output, &diagnostic) && diagnostic.code == -10 &&
         diagnostic.line == 2 && diagnostic.column == 5 &&
         diagnostic.offset == (long)(strstr(source, "Missing") - source) && output == SIZE_MAX - 1;
    ok = ok && !limbo_interface_resolve_type(owner, declaration, type, &output, &diagnostic) && diagnostic.code == -1 &&
         output == SIZE_MAX - 1;
    ok = ok && !limbo_interface_resolve_type(NULL, SIZE_MAX, type, &output, &diagnostic) && diagnostic.code == -1 &&
         output == SIZE_MAX - 1;
    limbo_interface_destroy(owner);
    return ok;
}


/* Cover forward chains, lexical shadowing, UTF-8 names, composites, cycles and output retention. */
int main(void)
{
    if (!diagnostics() || !check("M: module { A: type B; B: type big; x: A; };", "big", 0) ||
        !check("A: type big; B: type A; M: module { A: type real; x: B; };", "big", 0) ||
        !check("M: module { A: type big; N: module { x: A; }; };", "big", 0) ||
        !check("A: type big; M: module { A: type real; x: A; };", "real", 0) ||
        !check("M: module { Число: type big; x: Число; };", "big", 0) ||
        !check("M: module { A: type string; x: # before\n A # after\n; };", "string", 0) ||
        !check("M: module { A: type array of big; x: A; };", "array", 0) || !check("x: byte;", "byte", 0) ||
        !check("M: module { x: Missing; };", NULL, -10) || !check("M: module { A: int; x: A; };", NULL, -10) ||
        !check("M: module { x: Other.Value; };", NULL, -10) || !check("M: module { x: Other[int]; };", NULL, -10) ||
        !check("M: module { A: type A; x: A; };", NULL, -11) ||
        !check("M: module { A: type B; B: type A; x: A; };", NULL, -11) ||
        !check("A: type Inner; M: module { Inner: type big; x: A; };", NULL, -10) || !depth(LIMBO_ALIAS_MAX_DEPTH, 0) ||
        !depth(LIMBO_ALIAS_MAX_DEPTH + 1, -11))
    {
        return 1;
    }
    puts(
        "PASS alias resolution: grammar identifiers, lexical scope, forward/cyclic/limited chains, UTF-8 and retention");
    return 0;
}
