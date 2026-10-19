/* KeyPress native preparation and retained AVM1 share limits without rollback of completed handlers. */
#include "clip_keypress_fixture.hpp"
/* Sweep a small numeric budget to cover capture failure, partial delivery and completion. */
int main()
{
    FVM_Config cfg;FVM_Runtime *runtime=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&runtime)==FVM_OK);
    unsigned partial=0,failed=0,complete=0;
    for(unsigned limit=1;limit<=200;++limit)
    {
        FVM_Context *c=clip_context(runtime);FVM_Object *root=keypress_root(c),*actor=get(c,root,"actor").object;
        FVM_Object *second=input_duplicate(c,actor,"second",3);runtime->config.instruction_limit=limit;
        FVM_Result result=fvm_movieclip_dispatch_key_press(c,root,1);
        assert(result==FVM_OK || result==FVM_ERROR);
        FVM_Value first=get(c,actor,"seen"),last=get(c,second,"seen");
        if(result==FVM_OK)++complete;else
        {
            ++failed;
            if(first.type==FVM_VALUE_NUMBER && first.number==42 && last.type==FVM_VALUE_UNDEFINED)++partial;
        }
        input_clean(c);runtime->config.instruction_limit=0;
        assert(fvm_movieclip_dispatch_key_press(c,root,1)==FVM_OK);number(c,actor,"seen",42);number(c,second,"seen",42);
        fvm_context_destroy(c);
    }
    assert(failed && partial && complete);fvm_runtime_destroy(runtime);
}
