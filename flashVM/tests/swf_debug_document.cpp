/* Authoring debug tags preserve opaque bytes through GC and transfer without affecting playback. */
#define main document_fixture_main
#include "swf_document.cpp"
#undef main
#include "../src/swf/format_constants.h"
static Bytes debug_movie(unsigned kind,unsigned id_size=FVM_SWF_DEBUG_ID_BYTES,bool terminated=true)
{
    Bytes tags,password,id,empty;
    if(kind==FVM_SWF_TAG_ENABLE_DEBUGGER2) { password.push_back(0);password.push_back(0); }
    password.push_back(255);password.push_back('x');if(terminated)password.push_back(0);
    tag(tags,kind,password);for(unsigned i=0;i<id_size;++i)id.push_back((unsigned char)(255-i));
    tag(tags,FVM_SWF_TAG_DEBUG_ID,id);tag(tags,1,empty);tag(tags,1,empty);tag(tags,0,empty);return movie(tags);
}
static void inspect(FVM_Object *clip,unsigned kind)
{
    FVM_Object *frames=clip->resource_result.object->properties["frames"].object;
    assert(frames->properties["swf_debugger_tag"].number==kind);
    FVM_Object *password=frames->properties["swf_debugger_password"].object;
    assert(password->properties["length"].number==2 && password->indexed[0].number==255 && password->indexed[1].number=='x');
    FVM_Object *id=frames->properties["swf_debug_id"].object;
    assert(id->properties["length"].number==FVM_SWF_DEBUG_ID_BYTES);
    for(unsigned i=0;i<FVM_SWF_DEBUG_ID_BYTES;++i)assert(id->indexed[i].number==255-i);
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned kind=FVM_SWF_TAG_ENABLE_DEBUGGER;kind<=FVM_SWF_TAG_ENABLE_DEBUGGER2;kind+=FVM_SWF_TAG_ENABLE_DEBUGGER2-FVM_SWF_TAG_ENABLE_DEBUGGER) {
        FVM_Context *c=0,*other=0;assert(fvm_context_create(rt,&c)==FVM_OK && fvm_context_create(rt,&other)==FVM_OK && fvm_standard_install(c)==FVM_OK);
        Bytes swf=debug_movie(kind);FVM_SWFInfo info;info.struct_size=sizeof(info);
        assert(fvm_swf_inspect(swf.begin(),swf.size(),&info)==FVM_OK);
        FVM_Object *root=0;assert(fvm_movieclip_load_swf(c,swf.begin(),swf.size(),"root",&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK && fvm_gc_collect(c,0)==FVM_OK);inspect(root,kind);
        assert(fvm_movieclip_advance(c,root,1)==FVM_OK);
        FVM_Channel *channel=0;assert(fvm_channel_create(&channel)==FVM_OK);FVM_Value value,out;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=root;
        assert(fvm_channel_send_value(c,channel,&value,0)==FVM_OK && fvm_channel_receive_value(other,channel,&out,0)==FVM_OK);
        assert(fvm_gc_pin(other,out.object)==FVM_OK && fvm_gc_collect(other,0)==FVM_OK);inspect(out.object,kind);
        for(unsigned size=FVM_SWF_DEBUG_ID_BYTES-1;size<=FVM_SWF_DEBUG_ID_BYTES+1;size+=2) {
            swf=debug_movie(kind,size);assert(fvm_swf_inspect(swf.begin(),swf.size(),&info)==FVM_INVALID_ARGUMENT);
            assert(fvm_movieclip_load_swf(c,swf.begin(),swf.size(),"bad",&root)==FVM_INVALID_ARGUMENT);
        }
        swf=debug_movie(kind,FVM_SWF_DEBUG_ID_BYTES,false);assert(fvm_swf_inspect(swf.begin(),swf.size(),&info)==FVM_INVALID_ARGUMENT);
        fvm_channel_destroy(channel);fvm_context_destroy(other);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
