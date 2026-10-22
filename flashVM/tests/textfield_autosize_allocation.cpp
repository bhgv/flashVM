/* Every C++ allocation boundary preserves mode, text, dimensions and rooted native state. */
#define main allocation_reference_main
#include "textfield_counted_text_allocation.cpp"
#undef main
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned method=0;method<6;++method){bool success=false;unsigned failures=0;
  for(long boundary=0;boundary<4096 && !success;++boundary){
   FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);FVM_Object *field=counted_field(c),*format=0;FVM_Value value,arg,out,args[3],edit;
   assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK && fvm_gc_pin(c,format)==FVM_OK);
   fvm_value_number(&value,40);assert(fvm_object_set(c,format,"size",&value)==FVM_OK);fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;
   assert(fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_OK);
   if(method>=4){fvm_value_number(&value,54);assert(fvm_object_set(c,field,"_width",&value)==FVM_OK);fvm_value_boolean(&value,1);assert(fvm_object_set(c,field,"multiline",&value)==FVM_OK && fvm_object_set(c,field,"wordWrap",&value)==FVM_OK);}
   assert(fvm_value_string(&value,method>=4?"AVAV":"AV")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK);
   if(method){fvm_value_boolean(&value,1);assert(fvm_object_set(c,field,"autoSize",&value)==FVM_OK);}
   double width=counted_metric(c,field,"_width"),height=counted_metric(c,field,"_height");
   fvm_value_number(&value,20);assert(fvm_object_set(c,format,"size",&value)==FVM_OK);
   assert(fvm_value_string(&edit,"AVA")==FVM_OK);fvm_value_number(args,0);fvm_value_number(args+1,1);assert(fvm_value_string(args+2,"B")==FVM_OK);
   if(method==0)fvm_value_boolean(&value,1);else if(method==4)fvm_value_boolean(&value,0);else fvm_value_number(&value,30);
   allowance=boundary;FVM_Result result;
   if(method==0)result=fvm_object_set(c,field,"autoSize",&value);
   else if(method==1)result=fvm_object_set(c,field,"text",&edit);
   else if(method==2)result=fvm_object_call(c,field,"replaceText",3,args,&out);
   else if(method==3)result=fvm_object_call(c,field,"setTextFormat",1,&arg,&out);
   else result=fvm_object_set(c,field,method==4?"wordWrap":"_width",&value);
   allowance=-1;assert(result==FVM_OK || result==FVM_OUT_OF_MEMORY);success=result==FVM_OK;if(!success)++failures;
   assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth);
   double expected_width=method==1?76:method==3?28:method==4?100:method==5?30:52;
   double expected_height=method==3?26:method==5?186:48;
   assert(counted_metric(c,field,"_width")==(success?expected_width:width) && counted_metric(c,field,"_height")==(success?expected_height:height));
   assert(fvm_object_get(c,field,"autoSize",&out)==FVM_OK && !std::strcmp(out.string,method||success?"left":"none"));
   counted_text(c,field,success&&method==1?"AVA":success&&method==2?"BV":method>=4?"AVAV":"AV",success&&method==1?3:method>=4?4:2);
   fvm_context_destroy(c);
  }
  assert(success && failures);
 }
 fvm_runtime_destroy(rt);
#endif
 return 0;
}
