/* Script constructor prototypes retain original Object methods after replacement and GC. */
#define main named_scope_fixture_main
#include "avm1_function2_named_scope.cpp"
#undef main
#include "flashvm/property.h"
static FVM_Object *make(FVM_Context *c)
{ unsigned char body[]={0};FVM_Object *fn=0;assert(fvm_function_create_avm1(c,body,sizeof(body),0,0,0,&fn)==FVM_OK);return fn; }
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=5;version<=9;++version) {
        FVM_Context *c=0,*other=0;assert(fvm_context_create(rt,&c)==FVM_OK && fvm_context_create(rt,&other)==FVM_OK);
        if(version<=8) { unsigned char swf[]={'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK); }
        assert(fvm_standard_install(c)==FVM_OK);FVM_Value klass,original;assert(fvm_get_global(c,"Object",&klass)==FVM_OK && fvm_object_get(c,klass.object,"prototype",&original)==FVM_OK);
        FVM_Object *replacement=0;assert(fvm_object_create(c,&replacement)==FVM_OK);FVM_Value value,out;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=replacement;assert(fvm_object_set(c,klass.object,"prototype",&value)==FVM_OK);
        assert(fvm_object_get(c,klass.object,"prototype",&out)==FVM_OK && out.object==replacement);assert(fvm_gc_collect(c,0)==FVM_OK);
        FVM_Object *fn=make(c);assert(fvm_gc_pin(c,fn)==FVM_OK);FVM_Value proto;assert(fvm_object_get(c,fn,"prototype",&proto)==FVM_OK);
        assert(proto.object->prototype==(version<=8?original.object:0));
        if(version<=8) {
            unsigned flags=0;int deleted=1;assert(fvm_object_property_flags(c,fn,"prototype",&flags)==FVM_OK && flags==3);
            assert(fvm_object_delete(c,fn,"prototype",&deleted)==FVM_OK && !deleted);
            Bytes body;body.push_back(0x96);u16(body,5);body.push_back(7);body.push_back(77);body.push_back(0);body.push_back(0);body.push_back(0);body.push_back(0x3e);body.push_back(0);
            FVM_Object *getter=0;assert(fvm_function_create_avm1(c,body.begin(),body.size(),0,0,0,&getter)==FVM_OK);
            FVM_Value args[3];fvm_value_string(args,"answer");fvm_value_undefined(args+1);args[1].type=FVM_VALUE_OBJECT;args[1].object=getter;fvm_value_undefined(args+2);args[2].type=FVM_VALUE_NULL;
            assert(fvm_object_call(c,proto.object,"addProperty",3,args,&out)==FVM_OK && out.type==FVM_VALUE_BOOLEAN && out.boolean);
            FVM_Object *instance=0;assert(fvm_function_construct(c,fn,0,0,&instance)==FVM_OK);assert(fvm_object_get(c,instance,"answer",&out)==FVM_OK && out.type==FVM_VALUE_NUMBER && out.number==77);
            assert(fvm_gc_collect(c,0)==FVM_OK && fvm_object_get(c,proto.object,"answer",&out)==FVM_OK && out.number==77);
            FVM_Channel *channel=0;assert(fvm_channel_create(&channel)==FVM_OK);value.object=fn;assert(fvm_channel_send_value(c,channel,&value,0)==FVM_OK && fvm_channel_receive_value(other,channel,&out,0)==FVM_OK);
            FVM_Object *copy=out.object;assert(fvm_object_property_flags(other,copy,"prototype",&flags)==FVM_OK && flags==3);assert(fvm_gc_pin(other,copy)==FVM_OK && fvm_gc_collect(other,0)==FVM_OK);assert(fvm_function_construct(other,copy,0,0,&instance)==FVM_OK);assert(fvm_object_get(other,instance,"answer",&out)==FVM_OK && out.number==77);
            fvm_channel_destroy(channel);
        }
        fvm_context_destroy(other);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
