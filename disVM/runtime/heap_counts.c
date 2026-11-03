/* Checked native ulong ownership for the retained hosted heap implementation. */
#include "heap_counts.h"
#include "disvm/refcount.h"
#include <stdio.h>
#include <stdlib.h>


/* Fatal native invariant; message is borrowed static text. No managed error
 * callback can resume partially completed ownership mutations. */
static void invalid(const char *message)
{
    fprintf(stderr, "disvm Heap ownership invariant: %s\n", message);
    abort();
}


/* [GC:] Caller protects referent before retain. Only count metadata is touched;
 * marking and source/root publication remain separate caller obligations. */
void dvm_runtime_heap_retain(unsigned long *reference)
{
    if (dvm_refcount_retain_u(reference) != DVM_REFCOUNT_LIVE)
    {
        invalid("retain of zero/overflow count");
    }
}


/* [GC:] Consume owner without any counter read after successful primitive RMW.
 * LAST grants retirement, not permission to bypass legacy C-stack gchalt. */
int dvm_runtime_heap_release(unsigned long *reference)
{
    DVM_RefcountResult result = dvm_refcount_release_u(reference);
    if (result == DVM_REFCOUNT_REFUSED)
    {
        invalid("release of zero count");
    }
    return result == DVM_REFCOUNT_LAST;
}
