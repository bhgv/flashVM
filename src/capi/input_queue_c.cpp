#include "../core/runtime.hpp"
#include <climits>
extern "C" {
FVM_Result fvm_input_enqueue(FVM_Context *c, const FVM_InputEvent *e, FVM_InputPhase phase) {
    FVM_QueuedInput item;
    if (!c || !e || e->fingers==0 || e->fingers>16 ||
        e->gesture<FVM_GESTURE_TAP || e->gesture>FVM_GESTURE_WHEEL ||
        (phase!=FVM_INPUT_BEGIN && phase!=FVM_INPUT_UPDATE && phase!=FVM_INPUT_END))
        return FVM_INVALID_ARGUMENT;
    if (c->queued_input.size()>=1024) return FVM_ERROR;
    item.event=*e;
    item.phase=phase;
    try { c->queued_input.push_back(item); }
    catch (...) { return FVM_OUT_OF_MEMORY; }
    return FVM_OK;
}
FVM_Result fvm_input_pending(FVM_Context *c, unsigned int *out) {
    if (!c || !out) return FVM_INVALID_ARGUMENT;
    *out=(unsigned int)c->queued_input.size();
    return FVM_OK;
}
FVM_Result fvm_input_pump(FVM_Context *c, unsigned int limit, unsigned int *processed) {
    unsigned int count=0;
    if (processed) *processed=0;
    if (!c || c->pumping_input) return FVM_INVALID_ARGUMENT;
    c->pumping_input=true;
    /* Remove before invoking callbacks; callbacks may enqueue more work. */
    while (!c->queued_input.empty() && (!limit || count<limit)) {
        FVM_QueuedInput item=c->queued_input.front();
        c->queued_input.erase(c->queued_input.begin());
        FVM_Result r=fvm_input_dispatch(c,&item.event,item.phase);
        if (r!=FVM_OK) {
            c->pumping_input=false;
            if (processed) *processed=count;
            return r;
        }
        ++count;
    }
    c->pumping_input=false;
    if (processed) *processed=count;
    return FVM_OK;
}
}
