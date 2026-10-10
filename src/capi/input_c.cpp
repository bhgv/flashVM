#include "../core/runtime.hpp"
extern "C" {
FVM_Result fvm_input_set_rule(FVM_Context *c, const FVM_InputRule *rule) {
    size_t i;
    if (!c || !rule || rule->struct_size!=sizeof(FVM_InputRule) || rule->fingers==0 || rule->fingers>16 || rule->gesture<FVM_GESTURE_TAP || rule->gesture>FVM_GESTURE_WHEEL || rule->button<FVM_MOUSE_NONE || rule->button>FVM_MOUSE_WHEEL_DOWN) return FVM_INVALID_ARGUMENT;
    for (i=0;i<c->input_rules.size();++i) {
        FVM_InputRule &r=c->input_rules[i];
        if (r.gesture==rule->gesture && r.fingers==rule->fingers && r.direction==rule->direction) { r=*rule; return FVM_OK; }
    }
    try { c->input_rules.push_back(*rule); } catch (...) { return FVM_OUT_OF_MEMORY; }
    return FVM_OK;
}
FVM_Result fvm_input_map(FVM_Context *c, const FVM_InputEvent *e, FVM_MouseButton *out) {
    size_t i;
    if (!c || !e || !out || e->fingers==0) return FVM_INVALID_ARGUMENT;
    *out=FVM_MOUSE_NONE;
    /* Exact direction wins over wildcard; last registered rule wins for ties. */
    for (i=c->input_rules.size();i>0;--i) {
        const FVM_InputRule &r=c->input_rules[i-1];
        if (r.gesture==e->gesture && r.fingers==e->fingers && r.direction==e->direction) { *out=r.button; return FVM_OK; }
    }
    for (i=c->input_rules.size();i>0;--i) {
        const FVM_InputRule &r=c->input_rules[i-1];
        if (r.gesture==e->gesture && r.fingers==e->fingers && r.direction==0) { *out=r.button; return FVM_OK; }
    }
    return FVM_OK;
}
}
