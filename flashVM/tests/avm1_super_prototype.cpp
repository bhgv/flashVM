/* Loaded super projects ancestor prototype; custom getters retain actual this. */
#define main super_fixture_main
#include "avm1_super.cpp"
#undef main
#include "../src/core/runtime.hpp"
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=5;version<=9;++version) {
        FVM_Context *c=0,*other=0;assert(fvm_context_create(rt,&c)==FVM_OK && fvm_context_create(rt,&other)==FVM_OK);
        if(version<=8) { unsigned char swf[]={'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_load_memory(other,swf,sizeof(swf))==FVM_OK); }
        assert(fvm_standard_install(c)==FVM_OK);FVM_Value klass,object_proto;assert(fvm_get_global(c,"Object",&klass)==FVM_OK && fvm_object_get(c,klass.object,"prototype",&object_proto)==FVM_OK);
        Bytes empty;empty.push_back(0);FVM_Value base=define(c,"Base",0,0,empty),sub=define(c,"Sub",0,0,empty),third=define(c,"Third",0,0,empty);
        assert(fvm_object_set_prototype(c,prototype(c,base),object_proto.object)==FVM_OK);extend(c,"Sub","Base");extend(c,"Third","Sub");
        Bytes read;variable(read,"super");text(read,"__proto__");read.push_back(0x4e);result(read);FVM_Value method=define(c,"read",0,0,read);
        assert(fvm_object_set(c,prototype(c,base),"readBase",&method)==FVM_OK && fvm_object_set(c,prototype(c,sub),"readSub",&method)==FVM_OK && fvm_object_set(c,prototype(c,third),"readThird",&method)==FVM_OK);
        FVM_Object *instance=0;assert(fvm_function_construct(c,third.object,0,0,&instance)==FVM_OK && fvm_gc_pin(c,instance)==FVM_OK);
        FVM_Value out;assert(fvm_object_call(c,instance,"readBase",0,0,&out)==FVM_OK);
        if(version<=8)assert(out.type==FVM_VALUE_UNDEFINED);else assert(out.type==FVM_VALUE_OBJECT && out.object==prototype(c,third));
        assert(fvm_object_call(c,instance,"readSub",0,0,&out)==FVM_OK && out.type==FVM_VALUE_OBJECT && out.object==(version<=8?object_proto.object:prototype(c,third)));
        assert(fvm_object_call(c,instance,"readThird",0,0,&out)==FVM_OK && out.type==FVM_VALUE_OBJECT && out.object==(version<=8?prototype(c,base):prototype(c,third)));
        Bytes escape;variable(escape,"super");result(escape);method=define(c,"escape",0,0,escape);assert(fvm_object_set(c,prototype(c,third),"escape",&method)==FVM_OK && fvm_object_call(c,instance,"escape",0,0,&out)==FVM_OK);
        FVM_Object *facade=out.object;assert(fvm_gc_pin(c,facade)==FVM_OK && fvm_gc_collect(c,0)==FVM_OK);
        FVM_Value rendered;assert(fvm_value_to_string(c,&out,&rendered)==FVM_OK);
        assert(rendered.type==FVM_VALUE_STRING && !std::strcmp(rendered.string,"[object Object]"));
        FVM_Channel *channel=0;assert(fvm_channel_create(&channel)==FVM_OK);assert(fvm_channel_send_value(c,channel,&out,0)==FVM_OK && fvm_channel_receive_value(other,channel,&out,0)==FVM_OK);
        FVM_Object *copy=out.object;assert(fvm_gc_pin(other,copy)==FVM_OK && fvm_gc_collect(other,0)==FVM_OK);FVM_Object *copied_receiver=copy->resource_result.object->properties["receiver"].object;
        assert(fvm_object_get(other,copy,"__proto__",&out)==FVM_OK && out.type==FVM_VALUE_OBJECT && out.object==(version<=8?copied_receiver->prototype->prototype->prototype:copied_receiver->prototype));
        FVM_Object *replacement=0;assert(fvm_object_create(c,&replacement)==FVM_OK && fvm_object_set_prototype(c,replacement,object_proto.object)==FVM_OK && fvm_object_set_prototype(c,prototype(c,sub),replacement)==FVM_OK);
        assert(fvm_object_get(c,facade,"__proto__",&out)==FVM_OK && out.type==FVM_VALUE_OBJECT && out.object==(version<=8?replacement:prototype(c,third)));
        assert(fvm_object_set_prototype(c,prototype(c,sub),prototype(c,base))==FVM_OK);
        FVM_Value marker;fvm_value_number(&marker,91);assert(fvm_object_set(c,instance,"marker",&marker)==FVM_OK && fvm_object_define_accessor_rooted(c,prototype(c,base),"__proto__",getter,0,0,0)==FVM_OK);
        assert(fvm_object_call(c,instance,"readSub",0,0,&out)==FVM_OK && out.type==FVM_VALUE_NUMBER && out.number==91);
        fvm_channel_destroy(channel);fvm_context_destroy(other);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
