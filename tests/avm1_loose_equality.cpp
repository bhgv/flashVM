/* Loaded Equals2 follows SWF 5 preconversion and the Flash Player 7/8 NaN rule. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include "avm1_standard_types.cpp"
static unsigned calls,order[4];
static FVM_Result number(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *raw)
{ unsigned id=*(unsigned *)raw;order[calls++]=id;fvm_value_number(out,4);return FVM_OK; }
static FVM_Result self(FVM_Context *,FVM_Object *owner,unsigned,const FVM_Value *,FVM_Value *out,void *)
{ ++calls;fvm_value_undefined(out);out->type=FVM_VALUE_OBJECT;out->object=owner;return FVM_OK; }
static FVM_Result absent(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{ ++calls;fvm_value_undefined(out);return FVM_OK; }
static FVM_Value ref(FVM_Object *object)
{ FVM_Value v;fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=object;return v; }
static FVM_Value equality(FVM_Context *c,unsigned opcode,const FVM_Value &a,const FVM_Value &b)
{ assert(fvm_set_global(c,"left",&a)==FVM_OK);assert(fvm_set_global(c,"right",&b)==FVM_OK);Bytes body;global(body,"left");global(body,"right");body.push_back((unsigned char)opcode);return run(c,body); }
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=5;version<=8;++version)
    {
        FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',0,16,0,0,0,0x08,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
        FVM_Value a,b;fvm_value_number(&a,std::numeric_limits<double>::quiet_NaN());b=a;
        assert(equality(c,0x49,a,b).boolean);assert(equality(c,0x66,a,b).boolean);assert(!equality(c,0x0e,a,b).boolean);
        fvm_value_string(&b,"NaN");assert(!equality(c,0x49,a,b).boolean);
        FVM_Object *left=0,*right=0;assert(fvm_construct(c,"Object",0,0,&left)==FVM_OK);assert(fvm_construct(c,"Object",0,0,&right)==FVM_OK);unsigned ids[]={1,2};assert(fvm_object_define_method_rooted(c,left,"valueOf",number,ids,0)==FVM_OK);assert(fvm_object_define_method_rooted(c,right,"valueOf",number,ids+1,0)==FVM_OK);
        a=ref(left);b=ref(right);calls=0;bool same=equality(c,0x49,a,b).boolean;assert(same==(version==5));assert(calls==(version==5?2U:0U));if(version==5)assert(order[0]==2 && order[1]==1);
        assert(fvm_object_define_method_rooted(c,left,"valueOf",self,0,0)==FVM_OK);fvm_value_number(&b,0);calls=0;assert(!equality(c,0x49,a,b).boolean && calls==(version==5?2U:1U));
        fvm_value_boolean(&b,1);calls=0;assert(!equality(c,0x49,a,b).boolean && calls==(version==5?3U:1U));
        assert(fvm_object_define_method_rooted(c,left,"valueOf",absent,0,0)==FVM_OK);fvm_value_undefined(&b);calls=0;assert(equality(c,0x49,a,b).boolean && calls==1);
        FVM_Object *bare=0;assert(fvm_construct(c,"Object",0,0,&bare)==FVM_OK);assert(fvm_object_set(c,bare,"__proto__",&b)==FVM_OK);FVM_Object *parent=right;assert(fvm_object_get_prototype(c,bare,&parent)==FVM_OK && !parent);a=ref(bare);assert(equality(c,0x49,a,b).boolean);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
