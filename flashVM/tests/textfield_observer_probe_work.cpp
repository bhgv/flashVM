/* Large SWF6 descriptor tables consume shared work before edits publish; no observer executes during probing. */
#include "textfield_edit_events_fixture.hpp"
#include <cstdio>
#ifdef FVM_HAS_FONTS
static FVM_Result probe_work_callback(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *raw)
{++*(unsigned*)raw;fvm_value_undefined(out);return FVM_OK;}
#endif
int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);unsigned char movie[]={'F','W','S',6,16,0,0,0,8,0,0,12,1,0,0,0};assert(fvm_load_memory(c,movie,sizeof(movie))==FVM_OK);
    FVM_Object *field=scroll_field(c);FVM_Value value;fvm_value_number(&value,1);for(unsigned i=0;i<128;++i){char name[32];std::sprintf(name,"probe%03u",i);assert(fvm_object_set(c,field,name,&value)==FVM_OK);}
    unsigned calls=0;assert(fvm_object_define_method_rooted(c,field,"ONSCROLLER",probe_work_callback,&calls,field)==FVM_OK);
    bool present=true;rt->config.instruction_limit=16;assert(fvm_textfield_observer_present(c,field,&present)==FVM_ERROR && !present && !calls);rt->config.instruction_limit=0;
    assert(fvm_textfield_observer_present(c,field,&present)==FVM_OK && present && c->avm1_steps>=128 && !calls);
    unsigned before=0,after=0,success=0;
    for(unsigned limit=1;limit<=256;++limit)
    {
        rt->config.instruction_limit=0;assert(edit_text(c,field,"AV\nAV\nAV",8)==FVM_OK);calls=0;assert(fvm_value_string(&value,"A")==FVM_OK);rt->config.instruction_limit=limit;FVM_Result result=fvm_object_set(c,field,"text",&value);rt->config.instruction_limit=0;
        assert(result==FVM_OK || result==FVM_ERROR);double width=counted_metric(c,field,"textWidth");
        if(result==FVM_OK){++success;assert(width==24 && calls==1);}else if(width==48){++before;assert(!calls);counted_text(c,field,"AV\nAV\nAV",8);}else{++after;assert(width==24 && !calls);}
        assert(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);
    }
    assert(before && after && success && fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);fvm_runtime_destroy(rt);
#endif
    return 0;
}
