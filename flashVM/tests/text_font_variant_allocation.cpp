/* Each regular/styled registration remains unpublished until all C++ allocations succeed. */
#define main allocation_reference_main
#include "textfield_counted_text_allocation.cpp"
#undef main
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned variant=0;variant<4;++variant){bool success=false;unsigned failures=0;
  for(long boundary=0;boundary<4096 && !success;++boundary){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK && fvm_standard_text_install(c)==FVM_OK);
   allowance=boundary;FVM_Result result=fvm_standard_text_register_font_variant(c,"Geometry",variant,fvm_test_font,sizeof(fvm_test_font),0);allowance=-1;
   assert(result==FVM_OK || result==FVM_OUT_OF_MEMORY);success=result==FVM_OK;if(!success)++failures;
   assert(c->fonts.size()==(success?1:0) && c->default_font.empty()==(!success || variant!=0));
   if(!success)assert(fvm_standard_text_register_font_variant(c,"Geometry",variant,fvm_test_font,sizeof(fvm_test_font),0)==FVM_OK);
   assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth);fvm_context_destroy(c);
  }assert(success && failures);
 }
 fvm_runtime_destroy(rt);
#endif
 return 0;
}
