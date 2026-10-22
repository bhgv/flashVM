/* Flash8 admission and packed-style validation precede font import/native attachment. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include "textfield_counted_text.cpp"
#include "../src/standard/text_state.hpp"
#include "../src/standard/text_constants.h"
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *source=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&source)==FVM_OK);
 FVM_Object *field=counted_field(source),*format=0;FVM_Value v,arg,out;assert(fvm_construct(source,"TextFormat",0,0,&format)==FVM_OK);
 assert(fvm_value_string(&v,"justify")==FVM_OK && fvm_object_set(source,format,"align",&v)==FVM_OK);fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;
 assert(fvm_object_call(source,field,"setNewTextFormat",1,&arg,&out)==FVM_OK && fvm_value_string(&v,"A V A V")==FVM_OK && fvm_object_set(source,field,"text",&v)==FVM_OK);
 FVM_TextFieldState state;assert(fvm_textfield_snapshot(field,state)==FVM_OK && (state.current.alignment&FVM_TEXT_STYLE_JUSTIFY));
 unsigned long invalid[]={1,2,3,8,512};for(unsigned i=0;i<5;++i){FVM_TextFieldState bad=state;bad.current.alignment|=invalid[i];FVM_Object *o=0;assert(fvm_object_create(source,&o)==FVM_OK && fvm_textfield_restore(source,o,bad)==FVM_INVALID_ARGUMENT && !o->resource);}
 for(unsigned version=6;version<=8;++version){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
  FVM_Object *target=counted_field(c),*f=0,*restored=0;assert(fvm_construct(c,"TextFormat",0,0,&f)==FVM_OK);assert(fvm_value_string(&v,"justify")==FVM_OK && fvm_object_set(c,f,"align",&v)==FVM_OK);arg.object=f;
  assert(fvm_object_call(c,target,"setNewTextFormat",1,&arg,&out)==(version==8?FVM_OK:FVM_UNSUPPORTED));
  assert(fvm_object_create(c,&restored)==FVM_OK);restored->resource_kind=FVM_RESOURCE_TEXT_FIELD;
  assert(fvm_textfield_restore(c,restored,state)==(version==8?FVM_OK:FVM_UNSUPPORTED));if(version!=8)assert(!restored->resource);
  fvm_context_destroy(c);
 }
 fvm_context_destroy(source);fvm_runtime_destroy(rt);
#endif
 return 0;
}
