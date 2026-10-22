/* Automatic geometry policy and future format survive binary/channel transport and owner teardown. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include "textfield_counted_text.cpp"
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0,*other=0;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_context_create(rt,&other)==FVM_OK);
 FVM_Object *field=counted_field(c),*format=0;FVM_Value arg,value,out,root;
 assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK);
 fvm_value_number(&value,40);assert(fvm_object_set(c,format,"size",&value)==FVM_OK);
 fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;
 assert(fvm_value_string(&value,"AV")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK && fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==FVM_OK);
 fvm_value_number(&value,20);assert(fvm_object_set(c,format,"size",&value)==FVM_OK && fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_OK);
 fvm_value_boolean(&value,1);assert(fvm_object_set(c,field,"autoSize",&value)==FVM_OK);
 fvm_value_undefined(&root);root.type=FVM_VALUE_OBJECT;root.object=field;
 assert(fvm_standard_text_install(other)==FVM_OK);
 unsigned char *wire=0;size_t size=0;assert(fvm_value_serialize(c,&root,&wire,&size)==FVM_OK && fvm_value_deserialize(other,wire,size,&out)==FVM_OK);fvm_serialized_free(wire);
 FVM_Object *copy=out.object;assert(fvm_gc_pin(other,copy)==FVM_OK && counted_metric(other,copy,"_width")==52 && counted_metric(other,copy,"_height")==48);
 assert(fvm_object_get(other,copy,"autoSize",&value)==FVM_OK && value.type==FVM_VALUE_STRING && !std::strcmp(value.string,"left"));
 FVM_Channel *channel=0;assert(fvm_channel_create(&channel)==FVM_OK && fvm_channel_send_value(c,channel,&root,0)==FVM_OK && fvm_channel_receive_value(other,channel,&out,0)==FVM_OK);fvm_channel_destroy(channel);
 assert(counted_metric(other,out.object,"_width")==52 && fvm_object_get(other,out.object,"autoSize",&value)==FVM_OK && !std::strcmp(value.string,"left"));
 fvm_value_boolean(&value,0);assert(fvm_object_set(c,field,"autoSize",&value)==FVM_OK);size_t plain_size=0;
 assert(fvm_value_serialize(c,&root,&wire,&plain_size)==FVM_OK && size==plain_size);fvm_serialized_free(wire);
 fvm_context_destroy(c);assert(fvm_gc_collect(other,0)==FVM_OK);
 assert(fvm_value_string(&value,"AVA")==FVM_OK && fvm_object_set(other,copy,"text",&value)==FVM_OK && counted_metric(other,copy,"_width")==40 && counted_metric(other,copy,"_height")==26);
 fvm_context_destroy(other);fvm_runtime_destroy(rt);
#endif
 return 0;
}
