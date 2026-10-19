/* Depth order is independent of insertion order in the project-owned hash table. */
#include "clip_input_fixture.hpp"
static unsigned at;
static const char *expected[]={"actor","low","middle","high"};
static FVM_Result order(FVM_Context *,FVM_Object *,unsigned n,const FVM_Value *a,FVM_Value *out,void *)
{assert(n==1 && a[0].type==FVM_VALUE_STRING && at<4 && !std::strcmp(a[0].string,expected[at++]));fvm_value_undefined(out);return FVM_OK;}
int main()
{FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);FVM_Context *c=clip_context(rt);Bytes code;variable(code,"this");text(code,"_name");code.push_back(0x4e);integer(code,1);text(code,"order");code.push_back(0x3d);code.push_back(0x17);code.push_back(0);FVM_Object *root=clip_load(c,clip_movie(8,clip_event_table(8,8,code)));assert(fvm_movieclip_advance(c,root,1)==FVM_OK);FVM_Object *actor=get(c,root,"actor").object;input_duplicate(c,actor,"high",10);input_duplicate(c,actor,"low",1);input_duplicate(c,actor,"middle",5);assert(fvm_register_function(c,"order",order,0)==FVM_OK);at=0;assert(fvm_movieclip_dispatch_clip_input(c,root,8)==FVM_OK && at==4);input_clean(c);fvm_context_destroy(c);fvm_runtime_destroy(rt);}
