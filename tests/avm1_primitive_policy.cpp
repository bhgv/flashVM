/* AVM1 default/numeric primitive hints select one method, including the SWF 5 Date rule. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include "avm1_standard_types.cpp"
static unsigned values,strings,right_calls;
static FVM_Result self_value(FVM_Context *,FVM_Object *self,unsigned,const FVM_Value *,FVM_Value *out,void *)
{ ++values;fvm_value_undefined(out);out->type=FVM_VALUE_OBJECT;out->object=self;return FVM_OK; }
static FVM_Result number_value(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{ ++values;fvm_value_number(out,4);return FVM_OK; }
static FVM_Result text_value(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *raw)
{ ++strings;return fvm_value_string(out,(const char *)raw); }
static FVM_Result right_fail(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *,void *)
{ ++right_calls;return FVM_ERROR; }
static FVM_Value operation(FVM_Context *c,unsigned opcode,const FVM_Value &left,const FVM_Value &right)
{ assert(fvm_set_global(c,"left",&left)==FVM_OK);assert(fvm_set_global(c,"right",&right)==FVM_OK);Bytes b;global(b,"left");global(b,"right");b.push_back((unsigned char)opcode);return run(c,b); }
static FVM_Value ref(FVM_Object *object)
{ FVM_Value value;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=object;return value; }
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=5;version<=8;++version)
    {
        FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',0,16,0,0,0,0x08,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
        FVM_Object *object=0,*right=0;assert(fvm_construct(c,"Object",0,0,&object)==FVM_OK);assert(fvm_construct(c,"Object",0,0,&right)==FVM_OK);
        assert(fvm_object_define_method_rooted(c,object,"valueOf",self_value,0,0)==FVM_OK);assert(fvm_object_define_method_rooted(c,object,"toString",text_value,(void *)"TEXT",0)==FVM_OK);assert(fvm_object_define_method_rooted(c,right,"valueOf",right_fail,0,0)==FVM_OK);
        FVM_Value a=ref(object),b,result;fvm_value_number(&b,1);values=strings=0;result=operation(c,0x47,a,b);assert(result.type==FVM_VALUE_NUMBER && result.number!=result.number && values==2 && strings==0);
        fvm_value_string(&b,"_");values=strings=0;result=operation(c,0x47,a,b);assert(!std::strcmp(result.string,"TEXT_") && values==1 && strings==1);
        b=ref(right);values=strings=right_calls=0;assert(!operation(c,0x48,a,b).boolean && values==1 && strings==0 && right_calls==0);values=0;assert(!operation(c,0x67,b,a).boolean && values==1 && right_calls==0);
        FVM_Value zero;fvm_value_number(&zero,0);FVM_Object *date=0;assert(fvm_construct(c,"Date",1,&zero,&date)==FVM_OK);assert(fvm_object_define_method_rooted(c,date,"valueOf",number_value,0,0)==FVM_OK);assert(fvm_object_define_method_rooted(c,date,"toString",text_value,(void *)"DATE",0)==FVM_OK);
        a=ref(date);fvm_value_string(&b,"_");values=strings=0;result=operation(c,0x47,a,b);assert(!std::strcmp(result.string,version==5?"4_":"DATE_"));assert(values==(version==5?1U:0U) && strings==(version==5?0U:1U));
        fvm_value_number(&b,4);values=strings=0;assert(!operation(c,0x48,a,b).boolean && values==1 && strings==0);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
