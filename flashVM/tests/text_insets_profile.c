/* C89 callers distinguish property storage from font-enabled extent application. */
#include "flashvm/standard_text.h"
#include "flashvm/standard.h"
#include "fixtures/font_data.h"
#include <assert.h>
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *format=0;FVM_Value v,arg,out;FVM_Result result;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_text_install(c)==FVM_OK);
 result=fvm_standard_text_register_font(c,"Geometry",fvm_test_font,sizeof(fvm_test_font),0);
#ifdef FVM_HAS_FONTS
 assert(result==FVM_OK);
#else
 assert(result==FVM_UNSUPPORTED);
#endif
 assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK);fvm_value_number(&v,10);assert(fvm_object_set(c,format,"leftMargin",&v)==FVM_OK);fvm_value_number(&v,40);assert(fvm_object_set(c,format,"size",&v)==FVM_OK);assert(fvm_value_string(&arg,"AV")==FVM_OK);
 result=fvm_object_call(c,format,"getTextExtent",1,&arg,&out);
#ifdef FVM_HAS_FONTS
 assert(result==FVM_OK && fvm_object_get(c,out.object,"textFieldWidth",&v)==FVM_OK && v.number==62);
#else
 assert(result==FVM_UNSUPPORTED);
#endif
 fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
