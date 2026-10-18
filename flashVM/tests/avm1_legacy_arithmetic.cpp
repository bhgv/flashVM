/* Legacy arithmetic coercion, callback order and SWF 4 division error spelling. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include "avm1_standard_types.cpp"
#include "flashvm/gc.h"
static unsigned calls,order[2];
static FVM_Result convert(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *raw)
{ unsigned id=*(unsigned *)raw;order[calls++]=id;assert(fvm_gc_collect(c,0)==FVM_INVALID_ARGUMENT);fvm_value_number(out,id==1?9:2);return FVM_OK; }
static FVM_Result fail(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *,void *)
{ return FVM_ERROR; }
static FVM_Value op(FVM_Context *c,unsigned code,const FVM_Value &left,const FVM_Value &right)
{ assert(fvm_set_global(c,"left",&left)==FVM_OK);assert(fvm_set_global(c,"right",&right)==FVM_OK);Bytes b;global(b,"left");global(b,"right");b.push_back((unsigned char)code);return run(c,b); }
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=4;version<=8;++version)
    {
        FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);
        unsigned char swf[]={'F','W','S',0,16,0,0,0,0x08,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
        FVM_Value left,right,result;fvm_value_string(&left,"12");fvm_value_boolean(&right,1);
        assert(op(c,0x0a,left,right).number==13);assert(op(c,0x0b,left,right).number==11);
        assert(op(c,0x0c,left,right).number==12);assert(op(c,0x0d,left,right).number==12);
        fvm_value_number(&left,1);fvm_value_number(&right,0);result=op(c,0x0d,left,right);
        assert(version==4 ? result.type==FVM_VALUE_STRING && !std::strcmp(result.string,"#ERROR#") : result.type==FVM_VALUE_NUMBER && result.number==std::numeric_limits<double>::infinity());
        fvm_value_number(&right,-0.0);result=op(c,0x0d,left,right);
        assert(version==4 ? result.type==FVM_VALUE_STRING : result.number==-std::numeric_limits<double>::infinity());
        fvm_value_string(&left,"3suffix");fvm_value_number(&right,1);result=op(c,0x0a,left,right);
        assert(version==4 ? result.number==4 : result.number!=result.number);
        FVM_Object *a=0,*b=0;assert(fvm_construct(c,"Object",0,0,&a)==FVM_OK);assert(fvm_construct(c,"Object",0,0,&b)==FVM_OK);
        unsigned ids[]={1,2};assert(fvm_object_define_method_rooted(c,a,"valueOf",convert,ids,0)==FVM_OK);assert(fvm_object_define_method_rooted(c,b,"valueOf",convert,ids+1,0)==FVM_OK);
        fvm_value_undefined(&left);left.type=FVM_VALUE_OBJECT;left.object=a;right=left;right.object=b;
        for(unsigned code=0x0a;code<=0x0d;++code){calls=0;result=op(c,code,left,right);assert(calls==2);assert(order[0]==(code==0x0b||code==0x0d?2U:1U));assert(result.number==(code==0x0a?11:code==0x0b?7:code==0x0c?18:4.5));}
        assert(fvm_object_define_method_rooted(c,b,"valueOf",fail,0,0)==FVM_OK);
        assert(fvm_set_global(c,"left",&left)==FVM_OK);assert(fvm_set_global(c,"right",&right)==FVM_OK);
        Bytes bad;global(bad,"left");global(bad,"right");bad.push_back(0x0b);bad.push_back(0);calls=0;
        assert(fvm_avm1_execute(c,bad.begin(),bad.size())==FVM_ERROR && calls==0);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
