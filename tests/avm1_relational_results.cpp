/* Numeric relational actions return undefined for unordered values, with distinct coercion order. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include "avm1_standard_types.cpp"
static unsigned calls,order[2];
static FVM_Result convert(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *raw)
{ unsigned id=*(unsigned *)raw;order[calls++]=id;fvm_value_number(out,id);return FVM_OK; }
static FVM_Value compare(FVM_Context *c,unsigned opcode,const FVM_Value &a,const FVM_Value &b)
{ assert(fvm_set_global(c,"left",&a)==FVM_OK);assert(fvm_set_global(c,"right",&b)==FVM_OK);Bytes body;global(body,"left");global(body,"right");body.push_back((unsigned char)opcode);return run(c,body); }
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=5;version<=8;++version)
    {
        FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',0,16,0,0,0,0x08,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
        FVM_Value a,b;fvm_value_number(&a,std::numeric_limits<double>::quiet_NaN());fvm_value_number(&b,1);
        assert(compare(c,0x48,a,b).type==FVM_VALUE_UNDEFINED);assert(compare(c,0x67,a,b).type==FVM_VALUE_UNDEFINED);
        fvm_value_string(&a,"invalid");assert(compare(c,0x48,a,b).type==FVM_VALUE_UNDEFINED);assert(compare(c,0x67,b,a).type==FVM_VALUE_UNDEFINED);
        fvm_value_number(&a,1);assert(compare(c,0x48,a,b).type==FVM_VALUE_BOOLEAN && !compare(c,0x48,a,b).boolean);
        FVM_Object *left=0,*right=0;assert(fvm_construct(c,"Object",0,0,&left)==FVM_OK);assert(fvm_construct(c,"Object",0,0,&right)==FVM_OK);unsigned ids[]={1,2};assert(fvm_object_define_method_rooted(c,left,"valueOf",convert,ids,0)==FVM_OK);assert(fvm_object_define_method_rooted(c,right,"valueOf",convert,ids+1,0)==FVM_OK);
        fvm_value_undefined(&a);a.type=FVM_VALUE_OBJECT;a.object=left;b=a;b.object=right;
        calls=0;assert(compare(c,0x48,a,b).boolean && calls==2 && order[0]==1 && order[1]==2);
        calls=0;assert(!compare(c,0x67,a,b).boolean && calls==2 && order[0]==2 && order[1]==1);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
