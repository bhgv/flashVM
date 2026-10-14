/* With assignments/deletions use virtual membership without treating path fields as own slots. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include "avm1_standard_types.cpp"
#include "flashvm/property.h"
static void with_body(Bytes &out,const Bytes &body)
{ global(out,"child");out.push_back(0x94);out.push_back(2);out.push_back(0);out.push_back((unsigned char)body.size());out.push_back((unsigned char)(body.size()>>8));for(size_t i=0;i<body.size();++i)out.push_back(body[i]); }
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=5;version<=8;++version){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);FVM_Object *root=0,*child=0;assert(fvm_movieclip_create(c,"root",1,&root)==FVM_OK);FVM_Value args[2],value,out;fvm_value_string(args,"child");fvm_value_number(args+1,1);assert(fvm_object_call(c,root,"createEmptyMovieClip",2,args,&out)==FVM_OK);child=out.object;object(c,"child",child);fvm_value_string(&value,"outer");assert(fvm_set_global(c,"_root",&value)==FVM_OK);
 Bytes body;text(body,"_root");text(body,"shadow");body.push_back(0x1d);Bytes code;with_body(code,body);text(code,"done");run(c,code);assert(fvm_object_get(c,child,"_root",&out)==FVM_OK && out.type==FVM_VALUE_STRING && !strcmp(out.string,"shadow") && fvm_get_global(c,"_root",&out)==FVM_OK && !strcmp(out.string,"outer"));int removed=0;assert(fvm_object_delete(c,child,"_root",&removed)==FVM_OK && removed);
 body.clear();text(body,"removed");text(body,"_root");body.push_back(0x3b);body.push_back(0x1d);code.clear();with_body(code,body);global(code,"removed");out=run(c,code);assert(out.type==FVM_VALUE_BOOLEAN && !out.boolean && fvm_get_global(c,"_root",&out)==FVM_OK && out.type==FVM_VALUE_STRING && !strcmp(out.string,"outer"));fvm_context_destroy(c);}
 fvm_runtime_destroy(rt);return 0;
}
