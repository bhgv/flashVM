#define main allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#define main exceptions_fixture_main
#include "actionscript_exceptions.cpp"
#undef main
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
 unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
 Bytes body,caught,empty,code;integer(body,42);body.push_back(0x2a);string(caught,"observed");variable(caught,"error");caught.push_back(0x1d);attempt(code,body,caught,empty,1,"error");code.push_back(0);
 bool success=false;unsigned failures=0;
 for(long boundary=0;boundary<3000 && !success;++boundary){FVM_GCRootFrame *roots=c->gc_frames;size_t scopes=c->lexical_scopes.size();unsigned native=c->gc_native_depth,script=c->script_depth;allowance=boundary;
 FVM_Result r=fvm_avm1_execute(c,code.begin(),code.size());allowance=-1;assert(c->gc_frames==roots && c->lexical_scopes.size()==scopes && c->gc_native_depth==native && c->script_depth==script && !c->avm1_target_active);
 if(r==FVM_OK){assert(global(c,"observed")==42);success=true;}else{assert(r==FVM_OUT_OF_MEMORY);++failures;assert(fvm_exception_clear(c)==FVM_OK);}
 assert(fvm_gc_collect(c,0)==FVM_OK);}
 assert(success && failures>0);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
