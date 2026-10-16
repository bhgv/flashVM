/* String builders charge empty arguments and every emitted UTF-16 unit. */
#define FVM_STRING_WORK_FIXTURE_ONLY
#include "string_loaded_work.cpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=0;version<=8;++version){
  if(version && version<4)continue;FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);string_work_load(c,version);
  FVM_Object *str=string_work_make(c,"\xf0\x9f\x98\x80",4);FVM_Value args[3],out,klass;assert(fvm_get_global(c,"String",&klass)==FVM_OK);
  fvm_value_string(args,"");fvm_value_string(args+1,"x");
  rt->config.instruction_limit=4;assert(fvm_object_call(c,str,"concat",2,args,&out)==(version?FVM_ERROR:FVM_OK));if(version)assert(out.type==FVM_VALUE_UNDEFINED && c->avm1_steps==4);
  rt->config.instruction_limit=5;assert(fvm_object_call(c,str,"concat",2,args,&out)==FVM_OK && fvm_value_string_size(&out)==5 && !std::memcmp(out.string,"\xf0\x9f\x98\x80x",5));if(version)assert(c->avm1_steps==5);
  fvm_value_number(args,0xd83d);fvm_value_number(args+1,0xde00);fvm_value_number(args+2,0);
  rt->config.instruction_limit=5;assert(fvm_object_call(c,klass.object,"fromCharCode",3,args,&out)==(version?FVM_ERROR:FVM_OK));if(version)assert(out.type==FVM_VALUE_UNDEFINED && c->avm1_steps==5);
  rt->config.instruction_limit=6;assert(fvm_object_call(c,klass.object,"fromCharCode",3,args,&out)==FVM_OK && fvm_value_string_size(&out)==5 && !std::memcmp(out.string,"\xf0\x9f\x98\x80\0",5));
  if(version){assert(c->avm1_steps==6);c->avm1_execution_depth=1;c->avm1_steps=5;
   assert(fvm_object_call(c,klass.object,"fromCharCode",3,args,&out)==FVM_ERROR && c->avm1_steps==6 && c->avm1_execution_depth==1);
   rt->config.instruction_limit=0;c->avm1_steps=std::numeric_limits<unsigned long>::max();
   assert(fvm_object_call(c,str,"concat",0,0,&out)==FVM_ERROR && c->avm1_steps==std::numeric_limits<unsigned long>::max());c->avm1_execution_depth=0;
  }
  /* Empty arguments still consume work even though they append no text. */
  FVM_Object *empty=string_work_make(c,"",0);for(unsigned i=0;i<3;++i)fvm_value_string(args+i,"");rt->config.instruction_limit=2;
  assert(fvm_object_call(c,empty,"concat",3,args,&out)==(version?FVM_ERROR:FVM_OK));rt->config.instruction_limit=0;
  assert(fvm_object_call(c,klass.object,"fromCharCode",0,0,&out)==FVM_OK && fvm_value_string_size(&out)==0);
  assert(fvm_gc_collect(c,0)==FVM_OK);string_work_clean(c);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);
}
