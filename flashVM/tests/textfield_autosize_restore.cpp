/* Legacy built-in stubs upgrade; forged automatic dimensions never attach native state. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include "textfield_counted_text.cpp"
#include "../src/standard/text_state.hpp"
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0,*other=0;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_context_create(rt,&other)==FVM_OK);
 FVM_Object *field=counted_field(c),*format=0;FVM_Value arg,value,out,root;
 assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK);fvm_value_number(&value,40);assert(fvm_object_set(c,format,"size",&value)==FVM_OK);
 fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;assert(fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_OK);
 assert(fvm_value_string(&value,"AV")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK);
 FVM_NativeFunction retired=fvm_text_callback_from_id(205);FVM_AccessorEntry &legacy=field->accessors["autoSize"];
 legacy.getter=legacy.setter=retired;legacy.userdata=0;legacy.root=0;
 fvm_value_undefined(&root);root.type=FVM_VALUE_OBJECT;root.object=field;unsigned char *wire=0;size_t size=0;
 assert(fvm_standard_text_install(other)==FVM_OK && fvm_value_serialize(c,&root,&wire,&size)==FVM_OK && fvm_value_deserialize(other,wire,size,&out)==FVM_OK);fvm_serialized_free(wire);
 FVM_Object *copy=out.object;assert(fvm_gc_pin(other,copy)==FVM_OK && fvm_object_get(other,copy,"autoSize",&value)==FVM_OK && !std::strcmp(value.string,"none"));
 fvm_value_boolean(&value,1);assert(fvm_object_set(other,copy,"autoSize",&value)==FVM_OK && counted_metric(other,copy,"_width")==52);
 FVM_TextFieldState state;assert(fvm_textfield_snapshot(copy,state)==FVM_OK && state.auto_size_left);
 for(unsigned which=0;which<2;++which){FVM_TextFieldState bad=state;FVM_Object *object=0;if(which)bad.height+=1;else bad.width+=1;
  assert(fvm_object_create(other,&object)==FVM_OK && fvm_textfield_restore(other,object,bad)==FVM_INVALID_ARGUMENT && !object->resource);}
 /* Host captures must not be replaced, even with the retired callback identities. */
 FVM_AccessorEntry captured;captured.getter=captured.setter=retired;captured.root=copy;
 fvm_textfield_upgrade_accessor("autoSize",captured);assert(captured.getter==retired && captured.root==copy);
 captured.root=0;captured.userdata=&state;fvm_textfield_upgrade_accessor("autoSize",captured);assert(captured.getter==retired && captured.userdata==&state);
 fvm_context_destroy(c);assert(fvm_gc_collect(other,0)==FVM_OK && counted_metric(other,copy,"_width")==52);
 fvm_context_destroy(other);fvm_runtime_destroy(rt);
#endif
 return 0;
}
