/* [GC:] FlashVM: opt-in traced string marking, interior-view lookup and allocation-free sweep. */
#ifndef FVM_CORE_STRING_GC_HPP
#define FVM_CORE_STRING_GC_HPP /* [GC:] Guard internal string collector declarations. */
#include "runtime.hpp"


/* [GC:] One immutable allocation range captured before finalizers may grow the string arena. */
struct FVM_StringGCRange
{
    size_t address; /* [GC:] Flat-address allocation start, never dereferenced by the index. */
    size_t length;  /* [GC:] Counted payload bytes, excluding the final NUL terminator. */
    size_t index;   /* [GC:] Descriptor index in the pre-finalization arena snapshot. */
};


/* [GC:] A per-collection plan allocates before marking and publishes reclamation only after remark.
 */
class FVM_StringGCPlan
{
    FVM_Context *context_; /* [GC:] Owning VM thread/context; never shared between interpreters. */
    bool enabled_;         /* [GC:] Captured policy; changing policy during collection is rejected. */
    fvm_detail::vector<FVM_StringGCRange> ranges_; /* [GC:] Allocation starts sorted for view lookup. */
    fvm_detail::vector<unsigned char> live_;       /* [GC:] One mark byte per original string allocation. */

  public:


    /* [GC:] Capture allocation metadata before finalization, or allocate nothing in arena mode. */
    explicit FVM_StringGCPlan(FVM_Context *);


    /* [GC:] Clear string marks before initial tracing and resurrection tracing. */
    void reset();


    /* [GC:] Retain the allocation containing a reachable exact or interior string view. */
    void mark(const char *);


    /* [GC:] Free unmarked original buffers after resource release and compact both ownership
     * indexes. */
    void sweep();
};
#endif
