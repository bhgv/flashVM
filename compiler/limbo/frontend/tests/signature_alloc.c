/* Inject failure only in the semantic pass, preserving the already-owned catalog. */
#include <stdio.h>
#include "signature.h"
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
    if (!(limbo_interface_parse(source, sizeof(source) - 1, &owner, &diagnostic)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    reject_allocation = 1;
    if (!(!limbo_interface_validate_functions(owner, &diagnostic)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(diagnostic.code == -2 && allocations == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(limbo_interface_signature(owner, 1, &signature) && signature.parameter_count == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!strcmp(limbo_tree_source(limbo_interface_tree(owner)), source)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    reject_allocation = 0;
    allocations = 0;
    if (!(limbo_interface_validate_functions(owner, &diagnostic)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(allocations == 2 && diagnostic.code == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    limbo_interface_destroy(owner);
    return 0;
}
