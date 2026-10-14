/* Real DefineLocal2 qualified reads call a setter when the getter returns undefined. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include "avm1_standard_types.cpp"
#define main existing_declaration_main
#include "execution_declare_undefined_path.c"
#undef main
int main(){FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);for(unsigned v=4;v<=8;++v){FVM_Context *c=0;FVM_Object *holder=0;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)v;assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_construct(c,"Object",0,0,&holder)==FVM_OK);object(c,"holder",holder);accessors(c,holder);defined=0;unsigned before=sets;Bytes code;text(code,"holder:x");code.push_back(0x41);text(code,"done");run(c,code);assert(sets==before+1);fvm_context_destroy(c);}assert(gets==5 && sets==5);fvm_runtime_destroy(rt);return 0;}
