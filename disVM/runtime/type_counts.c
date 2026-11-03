/* Checked native Type counts used by the hosted retained heap implementation. */
#include "type_counts.h"
#include "disvm/refcount.h"
#include <stdio.h>
#include <stdlib.h>


/* Terminate on an impossible hosted ownership state. message is static borrowed
 * text. No managed callback/longjmp: continuing could publish corrupt ownership. */
static void invalid(const char *message)
{
    fprintf(stderr, "disvm Type ownership invariant: %s\n", message);
    abort();
}


/* [GC:] Retain protected aligned native count. No payload/type-field access,
 * allocation or heap admission change. Terminal refusal cannot leak into guest
 * recovery after a partially constructed object. */
void dvm_runtime_type_retain(int *reference)
{
    if (dvm_refcount_retain_i(reference) != DVM_REFCOUNT_LIVE)
    {
        invalid("retain of zero/negative/overflow count");
    }
}


/* [GC:] Last release transfers retirement, not automatic reclamation. No count
 * load or pointer dereference follows a successful primitive release. */
int dvm_runtime_type_release(int *reference)
{
    DVM_RefcountResult result = dvm_refcount_release_i(reference);
    if (result == DVM_REFCOUNT_REFUSED)
    {
        invalid("release of zero/negative count");
    }
    return result == DVM_REFCOUNT_LAST;
}


/* [GC:] Static Type baseline cannot be consumed by an element release. Drop
 * checked owner without touching storage after the primitive's successful RMW. */
void dvm_runtime_type_release_nonfinal(int *reference)
{
    if (dvm_runtime_type_release(reference))
    {
        invalid("static Type baseline consumed");
    }
}
