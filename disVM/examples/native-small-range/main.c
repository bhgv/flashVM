/* Compact three-span native arithmetic; buffers are independently owned here. */
#include "disvm/small_range.h"
#include "disvm/range_access.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* valid is a required resource/result predicate; message identifies the failure.
 * Return void on success, terminate this standalone example on invariant failure.
 * This is not a VM error/cleanup API and must not be used in a live Dis callback. */
static void require(int valid, const char *message)
{
    if (!valid)
    {
        fprintf(stderr, "small-range example: %s\n", message);
        exit(1);
    }
}


/* Own native storage/domain through both accesses, then retire all metadata/TLS.
 * No arguments or managed references. Return0 on success; failures terminate.
 * The overlapping result demonstrates capture-before-publication, not GC roots. */
int main(void)
{
    unsigned char bytes[64];
    unsigned char copy[64];
    uint64_t first = 7;
    uint64_t second = 35;
    uint64_t result;
    DVM_TripleAccess compact = {0};
    DVM_RangeAccess wide = {0};
    DVM_NativeContext *context;
    DVM_CellDomain *domain;
    require(sizeof(void *) == 8, "example requires native64 small-span width");
    memset(bytes, 0xa5, sizeof(bytes));
    memmove(bytes+1, &first, sizeof(first));
    memmove(bytes+17, &second, sizeof(second));
    context = dvm_native_context_create(sizeof(long), NULL);
    require(context && dvm_native_context_bind(context), "bind owner-local TLS");
    domain = dvm_cell_domain_create();
    require(domain != NULL, "create the shared canonical domain");

    /* Each span is at most pointer-width. Output starts one byte inside first;
     * capture both entire operands before writing any result byte. No callbacks,
     * waits, allocation or managed ownership work belongs inside this interval. */
    require(dvm_small_range_access_begin(domain, bytes+1, sizeof(first),
        bytes+17, sizeof(second), bytes+2, sizeof(result), &compact) == DVM_CELL_ACQUIRED,
        "claim both inputs and overlapping result");
    memmove(&first, bytes+1, sizeof(first));
    memmove(&second, bytes+17, sizeof(second));
    result = first + second;
    memmove(bytes+2, &result, sizeof(result));
    require(dvm_small_range_access_end(&compact), "release compact access");
    memmove(&result, bytes+2, sizeof(result));
    require(result == 42 && bytes[0] == 0xa5 && bytes[10] == 0xa5,
        "result and surrounding canaries");

    /* A wide access uses the SAME domain, so overlapping native competitors
     * serialize with compact accesses. The creator retains both buffers here. */
    require(dvm_range_access_begin(domain, bytes, sizeof(bytes), copy, sizeof(copy), &wide) == DVM_CELL_ACQUIRED,
        "claim wide copy in the same domain");
    memmove(copy, bytes, sizeof(copy));
    require(dvm_range_access_end(&wide), "release wide access");
    require(!memcmp(copy, bytes, sizeof(bytes)), "exact wide copy");
    require(dvm_cell_domain_destroy(&domain) && !domain, "retire quiescent domain");
    require(dvm_native_context_unbind(context) && dvm_native_context_destroy(context), "retire TLS owner");
    puts("PASS compact native arithmetic:42, overlap, wide-domain compatibility");
    return 0;
}
