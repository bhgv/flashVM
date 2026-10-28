/* Fault only the resolver's temporary name allocation, not parser/catalog storage. */
#include "resolve.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int reject = 1; /* Owner-local fault switch; no concurrent parser calls occur. */


/* Return failure while enabled, otherwise own ordinary malloc storage for the resolver. */
void *limbo_resolver_test_malloc(size_t size)
{
    return reject ? NULL : malloc(size);
}


/* An allocation error must preserve output and leave the immutable catalog reusable. */
int main(void)
{
    const char *text = "M: module { A: type big; x: A; };";
    LimboInterface *owner = NULL;
    LimboDiagnostic diagnostic;
    size_t module;
    size_t declaration;
    size_t type;
    size_t output = SIZE_MAX - 1;
    int ok;
    if (!limbo_interface_parse(text, strlen(text), &owner, &diagnostic) ||
        !limbo_interface_find(owner, SIZE_MAX, "M", &module) ||
        !limbo_interface_find(owner, module, "x", &declaration) ||
        !limbo_interface_declared_type(owner, declaration, &type))
    {
        limbo_interface_destroy(owner);
        return 1;
    }
    ok = !limbo_interface_resolve_type(owner, module, type, &output, &diagnostic) && diagnostic.code == -2 &&
         output == SIZE_MAX - 1;
    reject = 0;
    ok = ok && limbo_interface_resolve_type(owner, module, type, &output, &diagnostic) && diagnostic.code == 0 &&
         !strcmp(limbo_interface_type_kind(owner, output), "big");
    limbo_interface_destroy(owner);
    if (!ok)
    {
        fprintf(stderr, "alias allocation failure damaged output or catalog\n");
        return 1;
    }
    puts("PASS alias allocation rejection and immutable-catalog recovery");
    return 0;
}
