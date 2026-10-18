#define main existing_allocation_main
#include "execution_services_allocation.cpp"
#undef main
#include <cstring>
static FVM_Object *expected;
static FVM_Result body(FVM_Context *,FVM_Object *self,unsigned n,const FVM_Value *a,FVM_Value *out,void *)
{ assert(self==expected && n==1 && a[0].number==17);return fvm_value_string(out,"done"); }
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *owner=0,*fn=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_construct(c,"Object",0,0,&owner)==FVM_OK && fvm_function_create_native(c,body,0,0,&fn)==FVM_OK);expected=owner;
 FVM_Value name,value,arg,out;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=owner;assert(fvm_set_global(c,"owner",&value)==FVM_OK);value.object=fn;assert(fvm_object_set(c,owner,"method",&value)==FVM_OK);fvm_value_string(&name,"owner:method");fvm_value_number(&arg,17);bool success=false;unsigned failures=0;
 for(long boundary=0;boundary<3000 && !success;++boundary){FVM_GCRootFrame *roots=c->gc_frames;FVM_Object *old_fn=c->calling_function,*old_owner=c->calling_owner,*old_ctor=c->calling_constructor;unsigned native=c->gc_native_depth,script=c->script_depth;fvm_value_number(&out,99);allowance=boundary;
 FVM_Result r=fvm_exec_call_function(c,0,0,0,0,0,&name,1,&arg,&out);allowance=-1;assert(c->gc_frames==roots && c->calling_function==old_fn && c->calling_owner==old_owner && c->calling_constructor==old_ctor && c->gc_native_depth==native && c->script_depth==script);
 if(r==FVM_OK){assert(out.type==FVM_VALUE_STRING && !std::strcmp(out.string,"done"));success=true;}else{assert(r==FVM_OUT_OF_MEMORY && out.type==FVM_VALUE_NUMBER && out.number==99);++failures;}
 assert(fvm_gc_collect(c,0)==FVM_OK);}
 assert(success && failures>0);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
