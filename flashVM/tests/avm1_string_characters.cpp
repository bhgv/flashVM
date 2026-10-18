/* Ordinary and multibyte action variants use the same supported value operations. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include "avm1_standard_types.cpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 const unsigned char length_ops[]={0x14,0x31},code_ops[]={0x32,0x36},from_ops[]={0x33,0x37};
 for(unsigned version=4;version<=8;++version){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);FVM_Value in,out;fvm_value_string_bytes(&in,"a\0b",3);assert(fvm_set_global(c,"input",&in)==FVM_OK);
 for(unsigned i=0;i<2;++i){Bytes b;global(b,"input");b.push_back(length_ops[i]);out=run(c,b);assert(out.number==3);b.clear();global(b,"input");b.push_back(code_ops[i]);out=run(c,b);assert(out.number==97);b.clear();integer(b,65601);b.push_back(from_ops[i]);out=run(c,b);assert(out.type==FVM_VALUE_STRING && !strcmp(out.string,"A"));}
 fvm_context_destroy(c);}fvm_runtime_destroy(rt);return 0;
}
