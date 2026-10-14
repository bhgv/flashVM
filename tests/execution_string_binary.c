/* C89 explicit string operations retain conversion order, counted bytes and GC temporaries. */
#include "flashvm/execution.h"
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include <assert.h>
#include <string.h>
static unsigned calls,order[2],fail_id;
static FVM_Result convert(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *raw)
{
 unsigned id=*(unsigned *)raw;FVM_Value value;assert(calls<2);order[calls++]=id;assert(fvm_gc_collect(c,0)==FVM_OK && fvm_object_get(c,frame->receiver,"alive",&value)==FVM_OK && value.number==id);
 if(fail_id==id){fvm_value_number(&value,77);return fvm_exception_raise(c,&value);}fvm_value_string_bytes(&value,id==1?"a\0b":"c\0d",3);return fvm_value_to_string(c,&value,out);
}
int main(void)
{
 FVM_Config cfg;FVM_Runtime *rt;unsigned version;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(version=0;version<=8;++version){FVM_Context *c;FVM_Value a,b,out,value;FVM_Object *left,*right,*fn;unsigned ids[2]={1,2},i;const unsigned char code[]={0};const FVM_ExecutionCompare modes[]={FVM_EXEC_EQUAL,FVM_EXEC_LESS,FVM_EXEC_GREATER};unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};if(version && version<4)continue;
 assert(fvm_context_create(rt,&c)==FVM_OK);if(version){swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}assert(fvm_standard_install(c)==FVM_OK && fvm_gc_string_mode(c,FVM_GC_STRINGS_TRACED)==FVM_OK);
 fvm_value_string_bytes(&a,"a\0b",3);fvm_value_string_bytes(&b,"c\0d",3);assert(fvm_exec_string_add(c,&a,&b,&out)==FVM_OK && fvm_value_string_size(&out)==6 && !memcmp(out.string,"a\0bc\0d",6));
 fvm_value_string(&a,"\xed\xa0\x80");fvm_value_string(&b,"\xed\xb0\x80");assert(fvm_exec_string_add(c,&a,&b,&out)==FVM_OK && fvm_value_string_size(&out)==6 && !memcmp(out.string,"\xed\xa0\x80\xed\xb0\x80",6));
 fvm_value_string(&a,"\xf0\x90\x80\x80");fvm_value_string(&b,"\xed\xa0\x80\xed\xb0\x80");assert(fvm_exec_string_compare(c,FVM_EXEC_EQUAL,&a,&b,&out)==FVM_OK && out.boolean);fvm_value_string(&b,"\xee\x80\x80");assert(fvm_exec_string_compare(c,FVM_EXEC_LESS,&a,&b,&out)==FVM_OK && out.boolean);
 assert(fvm_construct(c,"Object",0,0,&left)==FVM_OK && fvm_construct(c,"Object",0,0,&right)==FVM_OK);
 for(i=0;i<2;++i){FVM_Object *obj=i?right:left;fvm_value_number(&value,ids[i]);assert(fvm_object_set(c,obj,"alive",&value)==FVM_OK && fvm_function_create_avm1(c,code,sizeof(code),0,0,0,&fn)==FVM_OK && fvm_function_set_body(c,fn,convert,ids+i,0)==FVM_OK);fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=fn;assert(fvm_object_set(c,obj,"toString",&value)==FVM_OK);}
 fvm_value_undefined(&a);a.type=FVM_VALUE_OBJECT;a.object=left;b=a;b.object=right;calls=0;assert(fvm_exec_string_add(c,&a,&b,&out)==FVM_OK && calls==2 && order[0]==2 && order[1]==1 && fvm_value_string_size(&out)==6 && !memcmp(out.string,"a\0bc\0d",6) && fvm_gc_pin(c,left)==FVM_OK && fvm_gc_pin(c,right)==FVM_OK);
 for(i=0;i<3;++i){calls=0;assert(fvm_exec_string_compare(c,modes[i],&a,&b,&out)==FVM_OK && out.type==FVM_VALUE_BOOLEAN && out.boolean==(modes[i]==FVM_EXEC_LESS) && calls==2 && order[0]==(modes[i]==FVM_EXEC_GREATER?1U:2U));}
 fail_id=2;calls=0;fvm_value_number(&out,99);assert(fvm_exec_string_add(c,&a,&b,&out)==FVM_EXCEPTION && out.number==99 && calls==1 && fvm_exception_clear(c)==FVM_OK);calls=0;assert(fvm_exec_string_compare(c,FVM_EXEC_GREATER,&a,&b,&out)==FVM_EXCEPTION && out.number==99 && calls==2 && fvm_exception_clear(c)==FVM_OK);fail_id=0;
 fvm_value_string(&a,"left");fvm_value_string(&b,"right");assert(fvm_exec_string_add(c,&a,&b,&a)==FVM_OK && !strcmp(a.string,"leftright"));fvm_value_number(&out,99);assert(fvm_exec_string_compare(c,FVM_EXEC_STRICT_EQUAL,&a,&b,&out)==FVM_INVALID_ARGUMENT && out.number==99);fvm_context_destroy(c);}
 fvm_runtime_destroy(rt);return 0;
}
