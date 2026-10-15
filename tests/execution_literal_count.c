/* C89 literal counts distinguish signed rejection, materialization limits and host errors. */
#include "flashvm/execution.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
typedef struct Case {double input;FVM_Result host,loaded;FVM_ValueType type;double normalized;} Case;
int main(void)
{
 Case cases[]={
  {0,FVM_OK,FVM_OK,FVM_VALUE_NUMBER,0},
  {1.9,FVM_INVALID_ARGUMENT,FVM_OK,FVM_VALUE_NUMBER,1},
  {-0.5,FVM_INVALID_ARGUMENT,FVM_OK,FVM_VALUE_UNDEFINED,0},
  {0,FVM_INVALID_ARGUMENT,FVM_OK,FVM_VALUE_NUMBER,0},
  {0,FVM_INVALID_ARGUMENT,FVM_OK,FVM_VALUE_UNDEFINED,0},
  {0,FVM_INVALID_ARGUMENT,FVM_OK,FVM_VALUE_UNDEFINED,0},
  {65536,FVM_OK,FVM_OK,FVM_VALUE_NUMBER,65536},
  {65536.9,FVM_INVALID_ARGUMENT,FVM_OK,FVM_VALUE_NUMBER,65536},
  {65537,FVM_INVALID_ARGUMENT,FVM_UNSUPPORTED,FVM_VALUE_UNDEFINED,0},
  {2147483647.0,FVM_INVALID_ARGUMENT,FVM_UNSUPPORTED,FVM_VALUE_UNDEFINED,0},
  {2147483648.0,FVM_INVALID_ARGUMENT,FVM_OK,FVM_VALUE_UNDEFINED,0}
 };
 FVM_Config cfg;FVM_Runtime *rt;unsigned profile,i;
 cases[3].input=strtod("NaN",0);cases[4].input=HUGE_VAL;cases[5].input=-HUGE_VAL;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(profile=0;profile<6;++profile)
 {
  FVM_Context *c;FVM_Value v,out;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
  assert(fvm_context_create(rt,&c)==FVM_OK);if(profile){swf[3]=(unsigned char)(profile+3);assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}
  for(i=0;i<sizeof(cases)/sizeof(cases[0]);++i)
  {
   FVM_Result status=profile?cases[i].loaded:cases[i].host;fvm_value_number(&v,cases[i].input);fvm_value_number(&out,99);
   assert(fvm_exec_literal_count(c,&v,&out)==status);
   if(status!=FVM_OK)assert(out.type==FVM_VALUE_NUMBER && out.number==99);
   else if(profile)assert(out.type==cases[i].type && (out.type==FVM_VALUE_UNDEFINED || out.number==cases[i].normalized));
   else assert(out.type==FVM_VALUE_NUMBER && out.number==cases[i].input);
  }
  fvm_value_string(&v,"1.9");assert(fvm_exec_literal_count(c,&v,&v)==(profile?FVM_OK:FVM_ERROR));if(profile)assert(v.type==FVM_VALUE_NUMBER && v.number==1);else assert(v.type==FVM_VALUE_STRING);
  fvm_value_undefined(&v);assert(fvm_exec_literal_count(c,&v,&out)==(profile?FVM_OK:FVM_ERROR));if(profile)assert(out.type==FVM_VALUE_NUMBER && out.number==0);
  fvm_value_number(&out,99);assert(fvm_exec_literal_count(c,0,&out)==FVM_INVALID_ARGUMENT && out.number==99 && fvm_exec_literal_count(0,&v,&out)==FVM_INVALID_ARGUMENT && out.number==99 && fvm_exec_literal_count(c,&v,0)==FVM_INVALID_ARGUMENT);
  fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);return 0;
}
