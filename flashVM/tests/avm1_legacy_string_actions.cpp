/* Counted UTF-16 legacy string actions, integer wrapping and collection during coercion. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include "avm1_standard_types.cpp"
#include "flashvm/extensions.h"
#include "flashvm/gc.h"
static unsigned calls,order[2];
static FVM_Result ordered(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *raw)
{ unsigned id=*(unsigned *)raw;order[calls++]=id;return fvm_value_string(out,id==1?"a":"b"); }
static FVM_Value unary(FVM_Context *c,unsigned opcode,const FVM_Value &value)
{ assert(fvm_set_global(c,"input",&value)==FVM_OK);Bytes b;global(b,"input");b.push_back((unsigned char)opcode);return run(c,b); }
static FVM_Value binary(FVM_Context *c,unsigned opcode,const FVM_Value &left,const FVM_Value &right)
{ assert(fvm_set_global(c,"left",&left)==FVM_OK);assert(fvm_set_global(c,"right",&right)==FVM_OK);Bytes b;global(b,"left");global(b,"right");b.push_back((unsigned char)opcode);return run(c,b); }
static FVM_Value extract(FVM_Context *c,unsigned opcode,const FVM_Value &value,double start,double count)
{ assert(fvm_set_global(c,"input",&value)==FVM_OK);FVM_Value number;fvm_value_number(&number,start);assert(fvm_set_global(c,"start",&number)==FVM_OK);fvm_value_number(&number,count);assert(fvm_set_global(c,"count",&number)==FVM_OK);Bytes b;global(b,"input");global(b,"start");global(b,"count");b.push_back((unsigned char)opcode);return run(c,b); }
static void script_method(FVM_Context *c,FVM_Object *owner,Bytes body)
{ body.push_back(0x3e);body.push_back(0);FVM_Object *fn=0;assert(fvm_function_create_avm1(c,body.begin(),body.size(),0,0,0,&fn)==FVM_OK);FVM_Value v;fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=fn;assert(fvm_object_set(c,owner,"toString",&v)==FVM_OK); }
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);assert(fvm_context_create(rt,&c)==FVM_OK);
    const unsigned char swf[]={'F','W','S',8,16,0,0,0,0x08,0,0,12,1,0,0,0};assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);assert(fvm_extensions_install(c)==FVM_OK);
    FVM_Value a,b,result;fvm_value_string_bytes(&a,"a\0b",3);fvm_value_string_bytes(&b,"c\0d",3);result=binary(c,0x21,a,b);assert(fvm_value_string_size(&result)==6 && !std::memcmp(result.string,"a\0bc\0d",6));
    assert(unary(c,0x14,a).number==3 && unary(c,0x31,a).number==3);
    result=extract(c,0x15,a,2,1);assert(fvm_value_string_size(&result)==1 && result.string[0]==0);
    assert(fvm_value_string_bytes(&a,"\xf0\x9f\x98\x80",4)==FVM_OK);const char pair[]={char(0xed),char(0xa0),char(0xbd),char(0xed),char(0xb8),char(0x80)};assert(fvm_value_string_bytes(&b,pair,sizeof(pair))==FVM_OK);
    assert(binary(c,0x13,a,b).boolean);assert(unary(c,0x14,a).number==2);
    result=extract(c,0x35,a,1,1);unsigned short unit=0;size_t length=0;assert(fvm_value_get_utf16(&result,&unit,1,&length)==FVM_OK && length==1 && unit==0xd83d);
    fvm_value_string(&a,"abcd");assert(!std::strcmp(extract(c,0x15,a,2,-1).string,"bcd"));assert(!std::strcmp(extract(c,0x15,a,-2,2).string,"ab"));result=extract(c,0x15,a,10,2);assert(fvm_value_string_size(&result)==0);
    assert(!std::strcmp(extract(c,0x15,a,4294967298.0,1).string,"b"));
    fvm_value_string(&a,"10");fvm_value_string(&b,"2");assert(binary(c,0x29,a,b).boolean && !binary(c,0x68,a,b).boolean);
    fvm_value_number(&a,4294967295.0);assert(unary(c,0x18,a).number==-1);fvm_value_number(&a,-1.9);assert(unary(c,0x18,a).number==-1);fvm_value_number(&a,std::numeric_limits<double>::infinity());assert(unary(c,0x18,a).number==0);
    FVM_Object *ordered_left=0,*ordered_right=0;assert(fvm_construct(c,"Object",0,0,&ordered_left)==FVM_OK);assert(fvm_construct(c,"Object",0,0,&ordered_right)==FVM_OK);unsigned ids[]={1,2};assert(fvm_object_define_method_rooted(c,ordered_left,"toString",ordered,ids,0)==FVM_OK);assert(fvm_object_define_method_rooted(c,ordered_right,"toString",ordered,ids+1,0)==FVM_OK);
    fvm_value_undefined(&a);a.type=FVM_VALUE_OBJECT;a.object=ordered_left;b=a;b.object=ordered_right;
    const unsigned comparisons[]={0x13,0x21,0x29,0x68};for(unsigned i=0;i<4;++i){calls=0;binary(c,comparisons[i],a,b);assert(calls==2 && order[0]==(comparisons[i]==0x68?1U:2U));}
    /* The second script conversion collects temporary strings from the first. */
    FVM_Object *left=0,*right=0;assert(fvm_construct(c,"Object",0,0,&left)==FVM_OK);assert(fvm_construct(c,"Object",0,0,&right)==FVM_OK);
    Bytes body;text(body,"collected");integer(body,0);global(body,"gc");text(body,"collect");body.push_back(0x52);body.push_back(0x1d);text(body,"LEFT");script_method(c,left,body);
    body.clear();text(body,"prefix");text(body,"suffix");body.push_back(0x21);script_method(c,right,body);
    fvm_value_undefined(&a);a.type=FVM_VALUE_OBJECT;a.object=left;b=a;b.object=right;
    assert(fvm_gc_string_mode(c,FVM_GC_STRINGS_TRACED)==FVM_OK);
    for(unsigned i=0;i<20;++i){result=binary(c,0x21,a,b);assert(!std::strcmp(result.string,"LEFTprefixsuffix"));FVM_Value collected;assert(fvm_get_global(c,"collected",&collected)==FVM_OK && collected.number>=1);assert(!binary(c,0x13,a,b).boolean);assert(binary(c,0x29,a,b).boolean);assert(binary(c,0x68,b,a).boolean);}
    const unsigned ops[]={0x13,0x14,0x15,0x18,0x21,0x29,0x31,0x35,0x68};for(unsigned i=0;i<sizeof(ops)/sizeof(*ops);++i){unsigned char empty[]={(unsigned char)ops[i],0};assert(fvm_avm1_execute(c,empty,sizeof(empty))==FVM_OK);}
    FVM_Context *legacy=0;assert(fvm_context_create(rt,&legacy)==FVM_OK);unsigned char older[sizeof(swf)];std::memcpy(older,swf,sizeof(swf));older[3]=5;assert(fvm_load_memory(legacy,older,sizeof(older))==FVM_OK);assert(fvm_standard_install(legacy)==FVM_OK);
    fvm_value_string(&a,"abcd");assert(unary(legacy,0x14,a).number==4);fvm_value_string(&a,"\xc3\xa9");assert(fvm_set_global(legacy,"input",&a)==FVM_OK);Bytes unsupported;global(unsupported,"input");unsupported.push_back(0x14);unsupported.push_back(0);assert(fvm_avm1_execute(legacy,unsupported.begin(),unsupported.size())==FVM_UNSUPPORTED);fvm_context_destroy(legacy);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
