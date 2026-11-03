/* Native byte-range exclusion; no managed pointer ownership is inferred. */
#include "disvm/range_access.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Stop before continuing a refused native operation or invalid resource state. */
static void require(int valid, const char *message)
{
    if (!valid)
    {
        fprintf(stderr, "range example: %s\n", message);
        exit(1);
    }
}


/* Both native arrays and the domain stay owned through every guard operation.
 * Byte copying owns no referent: use typed managed operations for Dis references.
 * Return0 after releasing guards/domain/TLS; failure stops with a diagnostic. */
int main(void)
{
    unsigned char source[4096];
    unsigned char destination[4096];
    unsigned int index;
    DVM_RangeAccess access = {0};
    DVM_NativeContext *context = dvm_native_context_create(sizeof(long), NULL);
    DVM_CellDomain *domain;
    for (index = 0; index < sizeof(source); index++)
    {
        source[index] = (unsigned char)(index * 13 + 7);
    }
    memset(destination, 0xa5, sizeof(destination));
    require(context && dvm_native_context_bind(context), "bind native owner");
    domain = dvm_cell_domain_create();
    require(domain != NULL, "create shared byte/cell domain");
    require(dvm_range_access_begin(domain, source + 1, 3000, destination + 3, 3000, &access) == DVM_CELL_ACQUIRED,
        "claim complete source/destination byte spans");
    /* Bounded payload work only: no VM calls, errors, waits, root publication
     * or owner teardown while native access metadata remains acquired. */
    memmove(destination + 3, source + 1, 3000);
    require(dvm_range_access_end(&access), "release before callbacks or teardown");
    require(!memcmp(destination + 3, source + 1, 3000) && destination[2] == 0xa5 && destination[3003] == 0xa5,
        "exact payload and untouched boundary bytes");
    require(dvm_cell_domain_destroy(&domain) && !domain, "retire externally quiescent domain");
    require(dvm_native_context_unbind(context) && dvm_native_context_destroy(context), "retire context");
    puts("PASS native range example:3000 exact bytes, unaligned spans, complete teardown");
    return 0;
}
