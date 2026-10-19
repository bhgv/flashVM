/* Reject every structural/numeric error in the local list before the first host notification. */
#include "sound_snapshot_fixture.hpp"
#include <limits>
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned kind=0;kind<10;++kind)
    {
        FVM_Context *c=tick_context(rt);SoundSnapshot p;FVM_Object *root=sound_snapshot_root(c,p);FVM_Value value;
        if(kind==0)p.list->indexed.erase(1);
        if(kind==1){value=p.list->indexed[1];p.list->indexed.erase(1);p.list->indexed[2]=value;}
        if(kind==2)p.list->indexed[1]=tick_object(root);
        if(kind==3){fvm_value_number(&value,-1);p.list->indexed[1]=value;}
        if(kind==4){fvm_value_number(&value,0.5);p.list->indexed[1]=value;}
        if(kind==5){fvm_value_number(&value,std::numeric_limits<double>::infinity());p.list->indexed[1]=value;}
        if(kind==6){fvm_value_number(&value,std::numeric_limits<double>::quiet_NaN());p.list->indexed[1]=value;}
        if(kind==7){fvm_value_number(&value,(double)c->swf_bytes.size());p.list->indexed[1]=value;}
        if(kind==8){for(unsigned long i=2;i<=FVM_MOVIECLIP_SOUND_MAX_FRAME_EVENTS;++i)p.list->indexed[i]=p.list->indexed[0];}
        if(kind==9){FVM_Object *foreign=0;FVM_Context *other=tick_context(rt);assert(fvm_object_create(other,&foreign)==FVM_OK);
            root->resource_result.object->properties["frames"].object->indexed[1].object->properties["sounds"]=tick_object(foreign);
            assert(fvm_movieclip_advance(c,root,1)==FVM_INVALID_ARGUMENT && !p.count);fvm_context_destroy(c);fvm_context_destroy(other);continue;}
        /* A dense prefix is structurally valid but incomplete for the canonical frame. */
        assert(fvm_movieclip_advance(c,root,1)==FVM_INVALID_ARGUMENT && !p.count);
        tick_clean(c);assert(!c->movieclip_sound_busy);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
