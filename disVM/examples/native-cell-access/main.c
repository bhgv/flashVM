/* Native pointer-cell range guards; referent lifetime remains caller-owned. */
#include "disvm/cell_access.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Stop on misuse; a failed begin owns no guards and is safe to report. */
static void require(int valid, const char *message)
{
    if (!valid)
    {
        fprintf(stderr, "cell example: %s\n", message);
        exit(1);
    }
}


/* Demonstrate unaligned native cell copying. This stack independently owns the
 * referent and both buffers until after end/domain/context teardown. Range
 * exclusion does not retain a pointer or grant managed Dis heap access. */
int main(void)
{
    DVM_NativeContext *context = dvm_native_context_create(sizeof(long), NULL);
    DVM_CellDomain *domain;
    DVM_CellAccess access = {0};
    unsigned char source[sizeof(void *) + 1];
    unsigned char destination[sizeof(void *) + 1];
    int payload = 7;
    void *pointer = &payload;
    void *empty = NULL;
    void *result;
    require(context && dvm_native_context_bind(context), "bind native owner");
    domain = dvm_cell_domain_create();
    require(domain != NULL, "create cell domain");
    /* Initialization happens before publication to any other caller. Byte-copy
     * pointer values so no unaligned typed dereference is required. */
    memmove(source + 1, &pointer, sizeof(pointer));
    memmove(destination + 1, &empty, sizeof(empty));
    require(dvm_cell_access_begin(domain, source + 1, destination + 1, &access) == DVM_CELL_ACQUIRED,
        "claim all source/destination boundary cells");
    /* Only bounded native work belongs here. No VM/error callbacks, allocation,
     * waits, ownership changes or context/world transitions may span access. */
    memmove(destination + 1, source + 1, sizeof(pointer));
    require(dvm_cell_access_end(&access), "release before callbacks or teardown");
    memmove(&result, destination + 1, sizeof(result));
    require(result == &payload && *(int *)result == 7, "native result and independent lifetime");
    require(dvm_cell_domain_destroy(&domain) && !domain, "retire quiescent domain");
    require(dvm_native_context_unbind(context) && dvm_native_context_destroy(context), "retire context");
    puts("PASS native cell access example: unaligned source/destination, payload=7");
    return 0;
}
