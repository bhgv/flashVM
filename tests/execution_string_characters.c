/* C89 string/character services preserve UTF-16, legacy limits, aliases and callback roots. */
#include "flashvm/execution.h"
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include <assert.h>
#include <string.h>
#include <math.h>
static int numeric,failure;
static FVM_Result convert(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *raw)
{
 FVM_Value value;(void)raw;assert(fvm_gc_collect(c,0)==FVM_OK && fvm_object_get(c,frame->receiver,"alive",&value)==FVM_OK && value.number==42);
 if(failure){fvm_value_number(&value,77);return fvm_exception_raise(c,&value);}if(numeric){fvm_value_number(out,65537);return FVM_OK;}return fvm_value_string_bytes(out,"a\0b",3);
}
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt;unsigned version;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(version=0;version<=8;++version){FVM_Context *c;FVM_Value in,out,value;FVM_Object *obj,*fn;const unsigned char code[]={0};unsigned short pair[]={0xd800,0xdc00};unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};int legacy=version && version<6;if(version && version<4)continue;
 assert(fvm_context_create(rt,&c)==FVM_OK);if(version){swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}assert(fvm_standard_install(c)==FVM_OK);
 fvm_value_string_bytes(&in,"a\0b",3);assert(fvm_exec_string_length(c,&in,&out)==FVM_OK && out.number==3 && fvm_exec_char_code(c,&in,&out)==FVM_OK && out.number==97);fvm_value_string(&in,"");assert(fvm_exec_char_code(c,&in,&out)==FVM_OK && out.number==0);
 fvm_value_undefined(&in);assert(fvm_exec_string_length(c,&in,&out)==FVM_OK && out.number==(version && version<7?0:9));
 assert(fvm_value_string_utf16(c,pair,2,&in)==FVM_OK);fvm_value_number(&out,99);assert(fvm_exec_string_length(c,&in,&out)==(legacy?FVM_UNSUPPORTED:FVM_OK) && out.number==(legacy?99:2));fvm_value_number(&out,99);assert(fvm_exec_char_code(c,&in,&out)==(legacy?FVM_UNSUPPORTED:FVM_OK) && out.number==(legacy?99:65533));
 fvm_value_number(&in,65537);assert(fvm_exec_char_from_code(c,&in,&in)==FVM_OK && in.type==FVM_VALUE_STRING && !strcmp(in.string,"\1"));fvm_value_number(&in,-65471);assert(fvm_exec_char_from_code(c,&in,&out)==FVM_OK && !strcmp(out.string,"A"));fvm_value_number(&in,HUGE_VAL);assert(fvm_exec_char_from_code(c,&in,&out)==FVM_OK && fvm_value_string_size(&out)==0);
 fvm_value_number(&in,55296);fvm_value_number(&out,99);assert(fvm_exec_char_from_code(c,&in,&out)==(legacy?FVM_UNSUPPORTED:FVM_OK));if(legacy)assert(out.number==99);else assert(fvm_value_string_size(&out)==3 && !memcmp(out.string,"\xef\xbf\xbd",3));
 assert(fvm_object_create(c,&obj)==FVM_OK && fvm_gc_pin(c,obj)==FVM_OK && fvm_function_create_avm1(c,code,sizeof(code),0,0,0,&fn)==FVM_OK && fvm_function_set_body(c,fn,convert,0,0)==FVM_OK);fvm_value_number(&value,42);assert(fvm_object_set(c,obj,"alive",&value)==FVM_OK);fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=fn;assert(fvm_object_set(c,obj,"toString",&value)==FVM_OK && fvm_object_set(c,obj,"valueOf",&value)==FVM_OK);in.type=FVM_VALUE_OBJECT;in.object=obj;numeric=0;
 assert(fvm_exec_string_length(c,&in,&out)==FVM_OK && out.number==3 && fvm_exec_char_code(c,&in,&out)==FVM_OK && out.number==97);numeric=1;assert(fvm_exec_char_from_code(c,&in,&out)==FVM_OK && !strcmp(out.string,"\1"));
 failure=1;numeric=0;fvm_value_number(&out,99);assert(fvm_exec_string_length(c,&in,&out)==FVM_EXCEPTION && out.number==99 && fvm_exception_clear(c)==FVM_OK && fvm_exec_char_code(c,&in,&out)==FVM_EXCEPTION && out.number==99 && fvm_exception_clear(c)==FVM_OK);numeric=1;assert(fvm_exec_char_from_code(c,&in,&out)==FVM_EXCEPTION && out.number==99 && fvm_exception_clear(c)==FVM_OK);failure=0;assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);}
 fvm_runtime_destroy(rt);return 0;
}
