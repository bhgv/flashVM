/* Checked Module counts without changing registry or code/data lifetime policy. */
#include "module_counts.h"
#include "disvm/refcount.h"
#include <stdio.h>
#include <stdlib.h>


/* Fatal ownership invariant. message borrows static text; no guest error can
 * resume a partly completed native ownership construction. */
static void invalid(const char *message)
{
    fprintf(stderr, "disvm Module ownership invariant: %s\n", message);
    abort();
}


/* [GC:] Protected source/registry entry must keep count storage alive. */
void dvm_runtime_module_retain(int *reference)
{
    if (dvm_refcount_retain_i(reference) != DVM_REFCOUNT_LIVE)
    {
        invalid("retain of zero/negative/overflow count");
    }
}


/* [GC:] No counter or Module field access follows the successful primitive RMW. */
int dvm_runtime_module_release(int *reference)
{
    DVM_RefcountResult result = dvm_refcount_release_i(reference);
    if (result == DVM_REFCOUNT_REFUSED)
    {
        invalid("release of zero/negative count");
    }
    return result == DVM_REFCOUNT_LAST;
}
