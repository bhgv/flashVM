/* GetVariable name coercion and SWF4 display filtering occur only in the action service. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include "avm1_standard_types.cpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);FVM_Value value;fvm_value_number(&value,42);assert(fvm_set_global(c,"17",&value)==FVM_OK);Bytes b;integer(b,17);b.push_back(0x1c);assert(run(c,b).number==42);
 FVM_Object *clip=0;assert(fvm_movieclip_create(c,"clip",1,&clip)==FVM_OK);object(c,"clip",clip);b.clear();global(b,"clip");value=run(c,b);assert(value.type==(version==4?FVM_VALUE_UNDEFINED:FVM_VALUE_OBJECT));fvm_context_destroy(c);}
 fvm_runtime_destroy(rt);return 0;
}
