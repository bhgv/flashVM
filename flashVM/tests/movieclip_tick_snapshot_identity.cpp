/* Captured clip identities survive same-depth replacement, while changed depth order waits one tick. */
#include "movieclip_tick_order_fixture.hpp"
/* Keep native mutation identities separate from observable callback order. */
struct Mutation
{
    FVM_Object *root; /* Pinned selected timeline root. */
    FVM_Object *victim; /* Snapshot child replaced at the same depth before delivery. */
    FVM_Object *moved; /* Snapshot child moved earlier without changing parent. */
    FVM_Object *replacement; /* Newly published clip deferred to the next tick. */
    unsigned calls; /* Ordinary later-child callbacks. */
    bool mutate; /* Perform one replacement/swap, then observe the next tick normally. */
};
/* The new same-depth instance cannot enter an already prepared parent snapshot. */
static FVM_Result later(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *raw)
{
    ++static_cast<Mutation *>(raw)->calls;fvm_value_undefined(out);return FVM_OK;
}
/* Replace one pending receiver and move another to an earlier depth. */
static FVM_Result first(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *raw)
{
    Mutation *m=static_cast<Mutation *>(raw);
    if(m->mutate)
    {
        m->mutate=false;m->replacement=ordered_child(c,m->root,"replacement",2);
        assert(fvm_object_define_method_rooted(c,m->replacement,"onEnterFrame",later,m,0)==FVM_OK);
        FVM_Value depth;fvm_value_number(&depth,-4);
        assert(fvm_object_call(c,m->moved,"swapDepths",1,&depth,out)==FVM_OK);
    }
    fvm_value_undefined(out);return FVM_OK;
}
/* Use a separate root to make exact per-tick depth order observable after swapDepths. */
int main()
{
    FVM_Config cfg;FVM_Runtime *runtime=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&runtime)==FVM_OK);
    FVM_Context *c=tick_context(runtime);FVM_Object *root=tick_root(c);
    FVM_Object *moved=ordered_child(c,root,"moved",8),*victim=ordered_child(c,root,"victim",2),*a=ordered_child(c,root,"first",1);
    Mutation m={root,victim,moved,0,0,true};
    assert(fvm_object_define_method_rooted(c,a,"onEnterFrame",first,&m,0)==FVM_OK);
    assert(fvm_object_define_method_rooted(c,victim,"onEnterFrame",later,&m,0)==FVM_OK);
    assert(fvm_object_define_method_rooted(c,moved,"onEnterFrame",later,&m,0)==FVM_OK);
    assert(fvm_movieclip_advance(c,root,1)==FVM_OK && m.calls==1 && m.replacement);
    OrderTrace trace={{root,moved,a,m.replacement,0,0,0,0},0,4};
    for(unsigned i=0;i<trace.total;++i)
        assert(fvm_object_define_method_rooted(c,trace.expected[i],"onEnterFrame",ordered,&trace,0)==FVM_OK);
    assert(fvm_movieclip_advance(c,root,1)==FVM_OK && trace.count==4);
    tick_clean(c);fvm_context_destroy(c);fvm_runtime_destroy(runtime);
}
