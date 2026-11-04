/* Exercise native guard extension on two caller-owned stack nodes. */
#include "walker.h"
#include <stdio.h>


/* No arguments. Own bare native context/domain and stack graph through every
 * operation; require zero guard depth before retirement. Return0 on success,
 * nonzero on explicit setup/semantic/cleanup failure. This creates no Dis heap. */
int main(void)
{
    DVM_NativeContext *context = dvm_native_context_create(sizeof(void *), NULL);
    DVM_CellDomain *domain;
    ExampleNode last = {NULL};
    ExampleNode first = {&last};
    ExampleNode *source = &first;
    unsigned long output = 99;
    int failed = 0;
    if (!context)
    {
        fputs("native walker: context allocation failed\n", stderr);
        return 1;
    }
    if (!dvm_native_context_bind(context))
    {
        fputs("native walker: context bind failed\n", stderr);
        if (!dvm_native_context_destroy(context))
        {
            fputs("native walker: unbound retirement failed\n", stderr);
        }
        return 1;
    }
    domain = dvm_cell_domain_create();
    if (!domain || example_chain_length(domain, &source, &output) != DVM_CELL_ACQUIRED || output != 2)
    {
        fputs("native walker: pair count failed\n", stderr);
        failed = 1;
    }
    if (!failed)
    {
        source = NULL;
        if (example_chain_length(domain, &source, &output) != DVM_CELL_ACQUIRED || output != 0)
        {
            fputs("native walker: empty count failed\n", stderr);
            failed = 1;
        }
    }
    if (!failed)
    {
        source = &first;
        last.tail = &first; /* Both stack objects remain independently alive. */
        output = 99;
        if (example_chain_length(domain, &source, &output) != DVM_CELL_REFUSED || output != 99)
        {
            fputs("native walker: cycle refusal failed\n", stderr);
            failed = 1;
        }
        last.tail = NULL;
    }
    if (dvm_native_context_object_count(context) || (domain && !dvm_cell_domain_destroy(&domain)) ||
        !dvm_native_context_unbind(context) || !dvm_native_context_destroy(context))
    {
        fputs("native walker: retirement failed\n", stderr);
        return 1;
    }
    if (failed)
    {
        return 1;
    }
    puts("PASS native chain snapshot, empty and cycle refusal");
    return 0;
}
