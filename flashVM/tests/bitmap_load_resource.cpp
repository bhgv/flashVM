#define FVM_BITMAP_SWF_FIXTURE_ONLY
#include "swf_bitmap_fill.cpp"
#include "flashvm/avm1.h"
#include "flashvm/execution_mode.h"
#include "../src/standard/bitmap_state.hpp"
static inline FVM_Value bitmap_reference(FVM_Object *object){FVM_Value v;fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=object;return v;}
static inline FVM_Object *lookup_bitmap_class(FVM_Context *c)
{
    FVM_Value flash;assert(fvm_get_global(c,"flash",&flash)==FVM_OK);return get(c,get(c,flash.object,"display").object,"BitmapData").object;
}
static inline Bytes exported_bitmap_movie()
{
    Bytes tags,empty,exports;tag(tags,20,lossless(5,2,1,bitmap_raw()));u16(exports,1);u16(exports,7);text(exports,"Tile");tag(tags,56,exports);
    tag(tags,1,empty);tag(tags,0,empty);return movie(tags,1);
}
static inline FVM_Value loaded_bitmap(FVM_Context *c,FVM_Object *klass,const char *name)
{
    FVM_Value argument,out;fvm_value_string(&argument,name);assert(fvm_object_call(c,klass,"loadBitmap",1,&argument,&out)==FVM_OK);return out;
}
static inline void loaded_pixels(FVM_Context *c,FVM_Object *bitmap)
{
    FVM_Value a[2],out;fvm_value_number(a,0);fvm_value_number(a+1,0);assert(fvm_object_call(c,bitmap,"getPixel32",2,a,&out)==FVM_OK);assert(out.type==FVM_VALUE_NUMBER && out.number==-65536);
    assert(fvm_gc_pin(c,bitmap)==FVM_OK);assert(fvm_gc_collect(c,0)==FVM_OK);assert(fvm_object_call(c,bitmap,"getPixel32",2,a,&out)==FVM_OK && out.number==-65536);
}
static inline void loaded_packet(FVM_Context *c,FVM_Object *bundle)
{
    assert(fvm_gc_pin(c,bundle)==FVM_OK);assert(fvm_gc_collect(c,0)==FVM_OK);FVM_Value root=get(c,bundle,"root"),klass=get(c,bundle,"klass");
    assert(fvm_set_global(c,"_root",&root)==FVM_OK);FVM_Value image=loaded_bitmap(c,klass.object,"Tile");assert(image.type==FVM_VALUE_OBJECT);loaded_pixels(c,image.object);
}
static inline void push_text(Bytes &code,const char *value){code.push_back(0x96);u16(code,(unsigned)std::strlen(value)+2);code.push_back(0);text(code,value);}
static inline Bytes bitmap_load_action()
{
    Bytes code;push_text(code,"this");code.push_back(0x1c);push_text(code,"loaded");push_text(code,"Tile");
    code.push_back(0x96);u16(code,5);code.push_back(7);u32(code,1);push_text(code,"BitmapClass");code.push_back(0x1c);
    push_text(code,"loadBitmap");code.push_back(0x52);code.push_back(0x4f);code.push_back(0);return code;
}
#ifndef FVM_BITMAP_LOAD_FIXTURE_ONLY
static void run(FVM_ExecutionMode mode)
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0,*other=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_context_create(rt,&other)==FVM_OK);
    assert(fvm_execution_set_mode(c,mode,0)==FVM_OK);assert(fvm_execution_set_mode(other,mode,0)==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
    FVM_Object *klass=lookup_bitmap_class(c);assert(loaded_bitmap(c,klass,"Tile").type==FVM_VALUE_NULL);
    Bytes swf=exported_bitmap_movie();FVM_Object *root=0;assert(fvm_movieclip_load_swf(c,swf.begin(),swf.size(),"root",&root)==FVM_OK);assert(fvm_gc_pin(c,root)==FVM_OK);
    FVM_Value root_value=bitmap_reference(root);assert(fvm_set_global(c,"_root",&root_value)==FVM_OK);
    FVM_Value first=loaded_bitmap(c,klass,"Tile"),second=loaded_bitmap(c,klass,"Tile");assert(first.type==FVM_VALUE_OBJECT && second.type==FVM_VALUE_OBJECT && first.object!=second.object);
    assert(fvm_gc_pin(c,first.object)==FVM_OK);assert(fvm_gc_pin(c,second.object)==FVM_OK);loaded_pixels(c,first.object);loaded_pixels(c,second.object);
    FVM_BitmapState *a=fvm_bitmap_pixels(first.object),*b=fvm_bitmap_pixels(second.object);assert(a && b && a!=b && a->pixels.begin()!=b->pixels.begin());a->pixels[0]=99;assert(b->pixels[0]==255);
    assert(loaded_bitmap(c,klass,"missing").type==FVM_VALUE_NULL);
    FVM_Value klass_value=bitmap_reference(klass);assert(fvm_set_global(c,"BitmapClass",&klass_value)==FVM_OK);
    FVM_Object *empty_root=0;assert(fvm_movieclip_create(c,"empty",1,&empty_root)==FVM_OK);FVM_Value empty_value=bitmap_reference(empty_root);assert(fvm_set_global(c,"_root",&empty_value)==FVM_OK);
    Bytes action=bitmap_load_action();assert(fvm_avm1_execute_receiver(c,action.begin(),action.size(),root)==FVM_OK);FVM_Value script_loaded=get(c,root,"loaded");assert(script_loaded.type==FVM_VALUE_OBJECT);loaded_pixels(c,script_loaded.object);
    assert(fvm_avm1_execute_receiver(c,action.begin(),action.size(),empty_root)==FVM_OK);assert(get(c,empty_root,"loaded").type==FVM_VALUE_NULL);
    FVM_Object *bundle=0;assert(fvm_object_create(c,&bundle)==FVM_OK);assert(fvm_gc_pin(c,bundle)==FVM_OK);assert(fvm_object_set(c,bundle,"root",&root_value)==FVM_OK);assert(fvm_object_set(c,bundle,"klass",&klass_value)==FVM_OK);
    FVM_Channel *channel=0;assert(fvm_channel_create(&channel)==FVM_OK);FVM_Value packet=bitmap_reference(bundle),copy;assert(fvm_channel_send_value(c,channel,&packet,0)==FVM_OK);assert(fvm_channel_receive_value(other,channel,&copy,0)==FVM_OK);
    assert(fvm_gc_pin(other,copy.object)==FVM_OK);assert(fvm_gc_collect(other,0)==FVM_OK);FVM_Value imported_root=get(other,copy.object,"root"),imported_class=get(other,copy.object,"klass");
    assert(fvm_set_global(other,"_root",&imported_root)==FVM_OK);FVM_Value imported=loaded_bitmap(other,imported_class.object,"Tile");assert(imported.type==FVM_VALUE_OBJECT);loaded_pixels(other,imported.object);fvm_channel_destroy(channel);
    FVM_Result opened=fvm_channel_open_file("load-bitmap-resource.bin","w",&channel);
    if(opened==FVM_OK){assert(fvm_channel_send_value(c,channel,&packet,0)==FVM_OK);fvm_channel_destroy(channel);
        assert(fvm_channel_open_file("load-bitmap-resource.bin","r",&channel)==FVM_OK);assert(fvm_channel_receive_value(other,channel,&copy,0)==FVM_OK);
        fvm_channel_destroy(channel);assert(std::remove("load-bitmap-resource.bin")==0);loaded_packet(other,copy.object);}else assert(opened==FVM_UNSUPPORTED);
#ifdef OS_PORTS
    FVM_ResourceBroker *broker=0;FVM_Channel *queue=0,*network=0;FVM_Object *wrapped=0;char *uri=0;
    assert(fvm_broker_start("127.0.0.1","127.0.0.1",0,&broker)==FVM_OK);assert(fvm_context_set_broker(c,broker)==FVM_OK);assert(fvm_channel_create(&queue)==FVM_OK);
    assert(fvm_extensions_wrap_channel(c,queue,&wrapped)==FVM_OK);assert(fvm_broker_export(c,wrapped,&uri)==FVM_OK);assert(fvm_channel_open_network(other,uri,&network)==FVM_OK);
    assert(fvm_channel_send_value(c,queue,&packet,0)==FVM_OK);assert(fvm_channel_receive_value(other,network,&copy,0)==FVM_OK);loaded_packet(other,copy.object);
    fvm_channel_release(network);fvm_broker_stop(broker);fvm_channel_release(queue);fvm_broker_release(broker);fvm_serialized_free(uri);
#endif
    FVM_Value nul_name,nul_out;assert(fvm_value_string_bytes(&nul_name,"Tile\0suffix",11)==FVM_OK);assert(fvm_set_global(c,"_root",&root_value)==FVM_OK);
    assert(fvm_object_call(c,klass,"loadBitmap",1,&nul_name,&nul_out)==FVM_OK && nul_out.type==FVM_VALUE_NULL);
    FVM_Value evil;fvm_value_string(&evil,"invalid");FVM_Value prototype=get(c,klass,"prototype");assert(fvm_object_set(c,klass,"prototype",&evil)==FVM_OK);assert(fvm_set_global(c,"_root",&root_value)==FVM_OK);
    FVM_Value argument,out;fvm_value_string(&argument,"Tile");assert(fvm_object_call(c,klass,"loadBitmap",1,&argument,&out)==FVM_INVALID_ARGUMENT);assert(fvm_object_set(c,klass,"prototype",&prototype)==FVM_OK);
    loaded_pixels(c,loaded_bitmap(c,klass,"Tile").object);fvm_context_destroy(other);fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
int main()
{
    run(FVM_EXECUTION_INTERPRETER);
    run(FVM_EXECUTION_MANUAL);
    run(FVM_EXECUTION_JIT);
    return 0;
}
#endif
