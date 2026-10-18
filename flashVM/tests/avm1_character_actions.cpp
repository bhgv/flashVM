/* Character actions: UTF-16, wrapping, empty/NUL input, coercion errors and old-version gates. */
#define main legacy_string_fixture_main
#include "avm1_legacy_string_actions.cpp"
#undef main
static FVM_Result fail_character(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *,void *)
{ return FVM_OUT_OF_MEMORY; }
static void unit(FVM_Context *c,unsigned op,double number,unsigned short expected)
{ FVM_Value in;fvm_value_number(&in,number);FVM_Value out=unary(c,op,in);unsigned short got=0;size_t length=999;assert(fvm_value_get_utf16(&out,&got,1,&length)==FVM_OK && length==(expected?1U:0U));if(expected)assert(got==expected); }
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=5;version<=8;++version){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
 FVM_Value in;for(unsigned op=0x32;op<=0x36;op+=4){fvm_value_string_bytes(&in,"\0A",2);assert(unary(c,op,in).number==0);fvm_value_string(&in,"");assert(unary(c,op,in).number==0);fvm_value_string(&in,"Abc");assert(unary(c,op,in).number==65);fvm_value_number(&in,123);assert(unary(c,op,in).number==49);
 unsigned short first=0xd83d;assert(fvm_value_string_utf16(c,&first,1,&in)==FVM_OK);assert(fvm_set_global(c,"input",&in)==FVM_OK);Bytes code;global(code,"input");code.push_back((unsigned char)op);code.push_back(0);assert(fvm_avm1_execute(c,code.begin(),code.size())==(version<6?FVM_UNSUPPORTED:FVM_OK));if(version>=6)assert(unary(c,op,in).number==65533);
 if(version>=6){fvm_value_string(&in,"\xf0\x9f\x98\x80");assert(unary(c,op,in).number==65533);fvm_value_string(&in,"\xc3\xa9");assert(unary(c,op,in).number==233);}}
 for(unsigned op=0x33;op<=0x37;op+=4){unit(c,op,0,0);unit(c,op,65.9,65);unit(c,op,65536,0);unit(c,op,65537,1);unit(c,op,std::numeric_limits<double>::infinity(),0);if(version>=6){unit(c,op,-1,65535);unit(c,op,0xd800,65533);unit(c,op,0xdfff,65533);unit(c,op,0x20ac,0x20ac);}}
 FVM_Object *obj=0;assert(fvm_construct(c,"Object",0,0,&obj)==FVM_OK);assert(fvm_object_define_method_rooted(c,obj,"toString",fail_character,0,0)==FVM_OK);assert(fvm_object_define_method_rooted(c,obj,"valueOf",fail_character,0,0)==FVM_OK);fvm_value_undefined(&in);in.type=FVM_VALUE_OBJECT;in.object=obj;assert(fvm_set_global(c,"input",&in)==FVM_OK);const unsigned ops[]={0x32,0x33,0x36,0x37};for(unsigned i=0;i<4;++i){Bytes code;global(code,"input");code.push_back((unsigned char)ops[i]);code.push_back(0);assert(fvm_avm1_execute(c,code.begin(),code.size())==FVM_OUT_OF_MEMORY);unsigned char empty[]={(unsigned char)ops[i],0};assert(fvm_avm1_execute(c,empty,sizeof(empty))==FVM_OK);}
 fvm_context_destroy(c);}
 fvm_runtime_destroy(rt);return 0;
}
