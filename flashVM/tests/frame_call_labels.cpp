/* Duplicate counted labels select the lowest frame regardless of insertion; lookup obeys work limits. */
#include "frame_blocks_fixture.hpp"
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=0;version<=8;++version)if(!version || version>=5)
    {
        FVM_Context *c=tick_context(rt,version);FVM_Object *clip=tick_root(c,3);Bytes high=action(3),low=action(1);
        assert(fvm_movieclip_define_frame(c,clip,3,"duplicate",high.begin(),high.size())==FVM_OK);
        assert(fvm_movieclip_define_frame(c,clip,1,"duplicate",low.begin(),low.size())==FVM_OK);
        FVM_Value label;assert(fvm_value_string(&label,"duplicate")==FVM_OK);
        rt->config.instruction_limit=10;assert(fvm_movieclip_call_frame(c,clip,label)==FVM_ERROR && c->avm1_steps==10);tick_clean(c);
        FVM_Value seen;assert(fvm_object_get(c,clip,"seen",&seen)==FVM_OK && seen.type==FVM_VALUE_UNDEFINED);
        rt->config.instruction_limit=0;assert(fvm_movieclip_call_frame(c,clip,label)==FVM_OK);number(c,clip,"seen",1);
        FVM_Object *frames=clip->resource_result.object->properties["frames"].object;
        FVM_Value counted;assert(fvm_value_string_bytes(&counted,"x\0label",7)==FVM_OK);
        frames->indexed[3].object->properties["label"]=counted;frames->indexed[1].object->properties["label"]=counted;
        assert(fvm_movieclip_call_frame(c,clip,counted)==FVM_OK);number(c,clip,"seen",1);
        tick_clean(c);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
