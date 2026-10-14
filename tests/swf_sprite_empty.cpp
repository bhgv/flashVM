/* Zero-frame sprite definitions retain their metadata through GC and transfer. */
#define main sprite_fixture_main
#include "swf_sprite_resources.cpp"
#undef main
#include "../src/core/runtime.hpp"
static void inspect(FVM_Object *root)
{
    FVM_Object *library=root->resource_result.object->properties["library"].object;
    FVM_Object *definition=library->properties["sprites"].object->indexed[7].object;
    assert(definition && definition->properties["total"].number==0 && definition->properties["id"].number==7);
    FVM_Object *frames=definition->properties["frames"].object;assert(frames && frames->indexed.empty());
    assert(library->properties["exports"].object->properties["ClassSymbol"].object==definition);
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=5;version<=8;++version) {
        FVM_Context *c=0,*other=0;assert(fvm_context_create(rt,&c)==FVM_OK && fvm_context_create(rt,&other)==FVM_OK);
        assert(fvm_standard_install(c)==FVM_OK);
        Bytes empty,controls,tags;tag(controls,0,empty);tag(controls,0,empty);tag(tags,39,sprite(7,0,controls));export_asset(tags,7,"ClassSymbol");
        tag(tags,12,action(19));tag(tags,1,empty);tag(tags,0,empty);Bytes swf=movie(tags,1);swf[3]=(unsigned char)version;
        FVM_Object *root=0;assert(fvm_movieclip_load_swf(c,swf.begin(),swf.size(),"root",&root)==FVM_OK);
        assert(fvm_gc_pin(c,root)==FVM_OK && fvm_gc_collect(c,0)==FVM_OK);inspect(root);
        assert(fvm_movieclip_advance(c,root,1)==FVM_OK);number(c,root,"seen",19);inspect(root);
        FVM_Channel *channel=0;assert(fvm_channel_create(&channel)==FVM_OK);
        FVM_Value value,out;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=root;
        assert(fvm_channel_send_value(c,channel,&value,0)==FVM_OK && fvm_channel_receive_value(other,channel,&out,0)==FVM_OK);
        assert(out.type==FVM_VALUE_OBJECT && fvm_gc_pin(other,out.object)==FVM_OK && fvm_gc_collect(other,0)==FVM_OK);inspect(out.object);
        assert(fvm_movieclip_advance(other,out.object,1)==FVM_OK);number(other,out.object,"seen",19);
        Bytes bad_controls,bad;tag(bad_controls,1,empty);tag(bad_controls,0,empty);
        tag(bad,39,sprite(7,0,bad_controls));tag(bad,1,empty);tag(bad,0,empty);rejects(c,bad,FVM_INVALID_ARGUMENT);
        bad_controls=controls;bad_controls.push_back(1);bad.clear();tag(bad,39,sprite(7,0,bad_controls));tag(bad,1,empty);tag(bad,0,empty);rejects(c,bad,FVM_INVALID_ARGUMENT);
        bad.clear();tag(bad,39,sprite(7,0,empty));tag(bad,1,empty);tag(bad,0,empty);rejects(c,bad,FVM_INVALID_ARGUMENT);
        bad.clear();tag(bad,39,sprite(7,0,controls));tag(bad,39,sprite(7,0,controls));tag(bad,1,empty);tag(bad,0,empty);rejects(c,bad,FVM_INVALID_ARGUMENT);
        fvm_channel_destroy(channel);fvm_context_destroy(other);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
