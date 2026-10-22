/* Four family slots are independent, regular default selection is stable and duplicates are atomic. */
#include "flashvm/standard_text.h"
#include "fixtures/font_data.h"
#include <assert.h>
#include <string.h>
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *field=0;unsigned int variant;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_text_install(c)==FVM_OK);
 assert(fvm_standard_text_register_font_variant(c,"Geometry",4,fvm_test_font,sizeof(fvm_test_font),0)==FVM_INVALID_ARGUMENT);
#ifdef FVM_HAS_FONTS
 for(variant=1;variant<=FVM_TEXT_FONT_VARIANTS;++variant){
  assert(fvm_standard_text_register_font_variant(c,"Geometry",variant,fvm_test_font,sizeof(fvm_test_font),0)==FVM_OK);
  assert(fvm_standard_text_register_font_variant(c,"Geometry",variant,fvm_test_font,sizeof(fvm_test_font),0)==FVM_INVALID_ARGUMENT);
 }
 assert(fvm_textfield_create(c,0,100,100,&field)==FVM_NOT_FOUND && !field);
 assert(fvm_standard_text_register_font(c,"Geometry",fvm_test_font,sizeof(fvm_test_font),0)==FVM_OK && fvm_textfield_create(c,0,100,100,&field)==FVM_OK);
 assert(fvm_standard_text_register_font_variant(c,"Geometry",FVM_TEXT_FONT_REGULAR,fvm_test_font,sizeof(fvm_test_font),0)==FVM_INVALID_ARGUMENT);
 { const char *names[]={"A","B","C"};unsigned int family;
  for(family=0;family<3;++family)for(variant=0;variant<=FVM_TEXT_FONT_VARIANTS;++variant)
   assert(fvm_standard_text_register_font_variant(c,names[family],variant,fvm_test_font,sizeof(fvm_test_font),0)==FVM_OK);
  assert(fvm_standard_text_register_font_variant(c,"D",0,fvm_test_font,sizeof(fvm_test_font),0)==FVM_UNSUPPORTED);
 }

#else
 for(variant=0;variant<=FVM_TEXT_FONT_VARIANTS;++variant)assert(fvm_standard_text_register_font_variant(c,"Geometry",variant,fvm_test_font,sizeof(fvm_test_font),0)==FVM_UNSUPPORTED);
#endif
 { char family[257];FVM_Context *edge=0;
  assert(fvm_context_create(rt,&edge)==FVM_OK && fvm_standard_text_install(edge)==FVM_OK);memset(family,'X',255);family[255]=0;
#ifdef FVM_HAS_FONTS
  { FVM_Object *format=0;FVM_Value v,arg,out;
   assert(fvm_standard_text_register_font(edge,family,fvm_test_font,sizeof(fvm_test_font),0)==FVM_OK);
   assert(fvm_standard_text_register_font_variant(edge,family,FVM_TEXT_FONT_VARIANTS,fvm_test_font,sizeof(fvm_test_font),0)==FVM_OK && fvm_textfield_create(edge,family,100,100,&field)==FVM_OK);
   assert(fvm_construct(edge,"TextFormat",0,0,&format)==FVM_OK);fvm_value_boolean(&v,1);assert(fvm_object_set(edge,format,"bold",&v)==FVM_OK && fvm_object_set(edge,format,"italic",&v)==FVM_OK);
   fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;assert(fvm_object_call(edge,field,"setTextFormat",1,&arg,&out)==FVM_OK);
  }
#else
  assert(fvm_standard_text_register_font_variant(edge,family,FVM_TEXT_FONT_VARIANTS,fvm_test_font,sizeof(fvm_test_font),0)==FVM_UNSUPPORTED);
#endif
  family[255]='X';family[256]=0;assert(fvm_standard_text_register_font_variant(edge,family,FVM_TEXT_FONT_VARIANTS,fvm_test_font,sizeof(fvm_test_font),0)==FVM_INVALID_ARGUMENT);fvm_context_destroy(edge);
 }
 fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
