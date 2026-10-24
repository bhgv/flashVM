/* Inject failure only in the semantic pass, preserving the already-owned catalog. */
#include "signature.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static int reject_allocation;
static size_t allocations;

/* The production signature pass frees successful allocations through normal free. */
void *limbo_signature_test_malloc(size_t size)
{
    ++allocations;
    return reject_allocation ? NULL : malloc(size);
}

/* Fail validation, inspect the same owner, then validate successfully with no reparse. */
int main(void)
{
    const char source[] = "M:module{first:fn(a,b:int);second:fn(x:string);};";
    LimboInterface *owner = NULL;
    LimboDiagnostic diagnostic;
    LimboSignature signature;
    assert(limbo_interface_parse(source, sizeof(source) - 1, &owner, &diagnostic));
    reject_allocation = 1;
    assert(!limbo_interface_validate_functions(owner, &diagnostic));
    assert(diagnostic.code == -2 && allocations == 1);
    assert(limbo_interface_signature(owner, 1, &signature) && signature.parameter_count == 2);
    assert(!strcmp(limbo_tree_source(limbo_interface_tree(owner)), source));
    reject_allocation = 0;
    allocations = 0;
    assert(limbo_interface_validate_functions(owner, &diagnostic));
    assert(allocations == 2 && diagnostic.code == 0);
    limbo_interface_destroy(owner);
    return 0;
}
