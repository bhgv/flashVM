/* Public loaded casing charges UCS2 mapping and UTF-16 output independently. */
#define FVM_STRING_WORK_FIXTURE_ONLY
#include "string_loaded_work.cpp"
#include "../src/standard/string_state.hpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=0;version<=8;++version){
  if(version && version<4)continue;FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);string_work_load(c,version);
  const char input[]={'A',0,(char)0xed,(char)0xa0,(char)0x80,'b'};FVM_Object *str=string_work_make(c,input,sizeof(input));FVM_Value out;
  const char *names[]={"toLowerCase","toUpperCase"};
  for(unsigned m=0;m<2;++m){rt->config.instruction_limit=7;assert(fvm_object_call(c,str,names[m],0,0,&out)==(version?FVM_ERROR:FVM_OK));
   if(version)assert(out.type==FVM_VALUE_UNDEFINED && c->avm1_steps==7);
   rt->config.instruction_limit=8;assert(fvm_object_call(c,str,names[m],0,0,&out)==FVM_OK && fvm_value_string_size(&out)==sizeof(input));
   assert(out.string[0]==(m?'A':'a') && out.string[1]==0 && !std::memcmp(out.string+2,input+2,3) && out.string[5]==(m?'B':'b'));
   if(version){assert(c->avm1_steps==8);c->avm1_execution_depth=1;c->avm1_steps=7;
    assert(fvm_object_call(c,str,names[m],0,0,&out)==FVM_ERROR && c->avm1_steps==8 && c->avm1_execution_depth==1);
    rt->config.instruction_limit=0;c->avm1_steps=std::numeric_limits<unsigned long>::max();
    assert(fvm_object_call(c,str,names[m],0,0,&out)==FVM_ERROR && c->avm1_steps==std::numeric_limits<unsigned long>::max());c->avm1_execution_depth=0;
    /* Internal property folding retains its caller's existing accounting policy. */
    FVM_Value text;assert(fvm_value_string_bytes(&text,input,sizeof(input))==FVM_OK);c->avm1_steps=17;
    assert(fvm_string_case(c,text,&out,m!=0)==FVM_OK && c->avm1_steps==17);
   }
   rt->config.instruction_limit=0;assert(fvm_gc_collect(c,0)==FVM_OK);string_work_clean(c);
  }
  if(version){FVM_Object *pair=string_work_make(c,"\xf0\x9f\x98\x80",4);rt->config.instruction_limit=3;
   assert(fvm_object_call(c,pair,"toLowerCase",0,0,&out)==FVM_ERROR && c->avm1_steps==3);rt->config.instruction_limit=4;
   assert(fvm_object_call(c,pair,"toLowerCase",0,0,&out)==FVM_OK && fvm_value_string_size(&out)==4 && c->avm1_steps==4);}
  rt->config.instruction_limit=0;fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);
}
