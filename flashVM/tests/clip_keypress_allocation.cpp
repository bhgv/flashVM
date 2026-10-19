/* KeyPress capture/selector/bytecode OOM sweep restores the outer input and collector scopes. */
#define main keypress_allocation_fixture_main
#include "swf_sprite_allocation.cpp"
#undef main
#include "clip_keypress_fixture.hpp"
/* Check a fresh usable broadcast after every failed native/AVM1 allocation. */
int main()
{
    FVM_Config cfg;FVM_Runtime *runtime=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&runtime)==FVM_OK);
    unsigned failures=0;bool complete=false;
    for(long n=0;n<4096 && !complete;++n)
    {
        FVM_Context *c=clip_context(runtime);FVM_Object *root=keypress_root(c),*actor=get(c,root,"actor").object;
        input_duplicate(c,actor,"second",3);allowance=n;
        FVM_Result result=fvm_movieclip_dispatch_key_press(c,root,1);allowance=-1;
        assert(result==FVM_OK || result==FVM_OUT_OF_MEMORY);input_clean(c);
        if(result==FVM_OK)complete=true;else{++failures;assert(fvm_movieclip_dispatch_key_press(c,root,1)==FVM_OK);}
        number(c,actor,"seen",42);number(c,get(c,root,"second").object,"seen",42);
        assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
    }
    assert(complete && failures);fvm_runtime_destroy(runtime);
}
