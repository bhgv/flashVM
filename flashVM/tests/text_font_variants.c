/* Four family slots are independent, regular default selection is stable and duplicates are atomic. */
#include "flashvm/standard_text.h"
#include "fixtures/font_data.h"
#include <assert.h>
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
 fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
