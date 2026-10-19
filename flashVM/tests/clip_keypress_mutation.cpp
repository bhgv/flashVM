/* KeyPress snapshots skip removed siblings, defer newborns and ignore visibility/playback filters. */
#include "clip_keypress_fixture.hpp"
/* Capture only test-native scene identities; the root pin retains their managed lifetime. */
struct Mutation
{
    FVM_Object *actor; /* First selected retained receiver. */
    FVM_Object *second; /* Later sibling removed before its turn. */
    FVM_Object *added; /* New sibling excluded from the first captured batch. */
    unsigned calls; /* Committed retained callbacks. */
};
/* Modify the display list through ordinary public MovieClip methods during retained execution. */
static FVM_Result mutate(FVM_Context *c,FVM_Object *,unsigned count,const FVM_Value *args,FVM_Value *out,void *raw)
{
    Mutation *mutation=static_cast<Mutation *>(raw);
    assert(count==1 && args[0].type==FVM_VALUE_OBJECT);++mutation->calls;
    if(!mutation->added)
    {
        mutation->added=input_duplicate(c,mutation->actor,"newborn",5);
        assert(fvm_object_call(c,mutation->second,"removeMovieClip",0,0,out)==FVM_OK);
    }
    FVM_Value value;fvm_value_number(&value,42);
    return fvm_object_set(c,args[0].object,"seen",&value);
}
/* A second broadcast captures the new tree; it never resumes a stored receiver cursor. */
int main()
{
    FVM_Config cfg;FVM_Runtime *runtime=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&runtime)==FVM_OK);
    FVM_Context *c=clip_context(runtime);Bytes code;
    variable(code,"this");integer(code,1);text(code,"mutateKey");code.push_back(0x3d);code.push_back(0x17);code.push_back(0);
    FVM_Object *root=keypress_root(c,8,1,code),*actor=get(c,root,"actor").object;
    Mutation mutation={actor,input_duplicate(c,actor,"later",3),0,0};
    assert(fvm_register_function(c,"mutateKey",mutate,&mutation)==FVM_OK);
    FVM_Value value,out;fvm_value_number(&value,0);assert(fvm_object_set(c,actor,"_visible",&value)==FVM_OK);
    assert(fvm_object_call(c,actor,"stop",0,0,&out)==FVM_OK);
    assert(fvm_movieclip_dispatch_key_press(c,root,1)==FVM_OK && mutation.calls==1 && mutation.added);
    number(c,actor,"seen",42);assert(get(c,mutation.second,"seen").type==FVM_VALUE_UNDEFINED);
    assert(get(c,mutation.added,"seen").type==FVM_VALUE_UNDEFINED);
    assert(fvm_movieclip_dispatch_key_press(c,root,1)==FVM_OK && mutation.calls==3);
    number(c,mutation.added,"seen",42);input_clean(c);
    assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);fvm_runtime_destroy(runtime);
}
