/* Measured geometry limits reject edits/activation atomically and permit recovery. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include "textfield_counted_text.cpp"
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
 FVM_Object *field=counted_field(c),*format=0;FVM_Value value,arg,out;char tall[512],wide[600];
 assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK && fvm_gc_pin(c,format)==FVM_OK);fvm_value_number(&value,4096);assert(fvm_object_set(c,format,"size",&value)==FVM_OK);
 fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;assert(fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_OK);
 fvm_value_boolean(&value,1);assert(fvm_object_set(c,field,"multiline",&value)==FVM_OK);
 assert(fvm_value_string(&value,"AV")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK);
 fvm_value_boolean(&value,1);assert(fvm_object_set(c,field,"autoSize",&value)==FVM_OK);
 double width=counted_metric(c,field,"_width"),height=counted_metric(c,field,"_height");
 for(unsigned i=0;i<256;++i){tall[2*i]='A';tall[2*i+1]='\n';}std::memset(wide,'A',sizeof(wide));
 assert(fvm_value_string_bytes(&value,tall,sizeof(tall))==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_UNSUPPORTED);
 counted_text(c,field,"AV",2);assert(counted_metric(c,field,"_width")==width && counted_metric(c,field,"_height")==height);
 assert(fvm_value_string_bytes(&value,wide,sizeof(wide))==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_UNSUPPORTED);
 counted_text(c,field,"AV",2);assert(counted_metric(c,field,"_width")==width && counted_metric(c,field,"_height")==height);
 fvm_value_boolean(&value,0);assert(fvm_object_set(c,field,"autoSize",&value)==FVM_OK);
 assert(fvm_value_string_bytes(&value,tall,sizeof(tall))==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK);
 fvm_value_boolean(&value,1);assert(fvm_object_set(c,field,"autoSize",&value)==FVM_UNSUPPORTED);
 assert(fvm_object_get(c,field,"autoSize",&value)==FVM_OK && !std::strcmp(value.string,"none") && counted_metric(c,field,"_width")==width);
 assert(fvm_value_string(&value,"A")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK);fvm_value_boolean(&value,1);assert(fvm_object_set(c,field,"autoSize",&value)==FVM_OK);
 assert(counted_metric(c,field,"_width")==counted_metric(c,field,"textWidth")+4 && counted_metric(c,field,"_height")==counted_metric(c,field,"textHeight")+4);
 assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth);fvm_context_destroy(c);fvm_runtime_destroy(rt);
#endif
 return 0;
}
