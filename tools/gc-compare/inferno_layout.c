/* Audit retained Pool leader metadata under the native ABI; do not run a fake pool. */
#include "lib9.h"
#include "isa.h"
#include "interp.h"
#include "pool.h"
#include <stdio.h>
#ifndef GC_POOL_HEAP_QUANTA
#error Supply the heap quantum read from the exact selected allocator source.
#endif

/* The real allocator writes a Btail at leader_size-sizeof(Btail).
 * Check whether that overwrites the leader's arena chain-size metadata. */
int main(void)
{
    size_t leader = GC_POOL_HEAP_QUANTA + 1;
    size_t tail = leader - sizeof(Btail);
    size_t chain_size = offsetof(Bhdr, u.l.size);
    size_t chain_end = chain_size + sizeof(((Bhdr *)0)->u.l.size);
    int overlaps = tail < chain_end && chain_size < tail + sizeof(Btail);
    printf("{\"native_pointer_bytes\":%lu,\"native_WORD_bytes\":%lu,\"leader_bytes\":%lu,"
           "\"tail_offset\":%lu,\"chain_size_offset\":%lu,\"chain_size_bytes\":%lu,"
           "\"leader_tail_overlap\":%s,\"layout_gate\":\"%s\"}\n",
           (unsigned long)sizeof(void *), (unsigned long)sizeof(WORD), (unsigned long)leader,
           (unsigned long)tail, (unsigned long)chain_size,
           (unsigned long)sizeof(((Bhdr *)0)->u.l.size), overlaps ? "true" : "false",
           overlaps ? "FAIL" : "PASS_LAYOUT_ONLY");
    return 0;
}
