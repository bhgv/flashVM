/* Depth snapshot is taken after parent delivery; detached children skip later visits. */
#include "movieclip_tick_fixture.hpp"
struct Mutation {FVM_Object *root,*victim,*later;unsigned order[8],count;Mutation():root(0),victim(0),later(0),count(0){} };
static FVM_Result child_call(FVM_Context *c,FVM_Object *self,unsigned,const FVM_Value *,FVM_Value *out,void *raw)
{ Mutation *p=(Mutation *)raw;p->order[p->count++]=self==p->victim?2:self==p->later?3:1;fvm_value_undefined(out);return FVM_OK; }
static FVM_Result first(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *raw)
{ Mutation *p=(Mutation *)raw;p->order[p->count++]=1;assert(fvm_object_call(c,p->victim,"removeMovieClip",0,0,out)==FVM_OK);FVM_Object *newborn=tick_child(c,p->root,"newborn",4);assert(fvm_object_define_method_rooted(c,newborn,"onEnterFrame",child_call,p,0)==FVM_OK);return FVM_OK; }
static FVM_Result parent_call(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *raw)
{ Mutation *p=(Mutation *)raw;p->order[p->count++]=0;p->later=tick_child(c,p->root,"later",3);assert(fvm_object_define_method_rooted(c,p->later,"onEnterFrame",child_call,p,0)==FVM_OK);fvm_value_undefined(out);return FVM_OK; }
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);FVM_Context *c=tick_context(rt);Mutation p;p.root=tick_root(c);FVM_Object *a=tick_child(c,p.root,"first",1);p.victim=tick_child(c,p.root,"victim",2);
 assert(fvm_object_define_method_rooted(c,p.root,"onEnterFrame",parent_call,&p,0)==FVM_OK && fvm_object_define_method_rooted(c,a,"onEnterFrame",first,&p,0)==FVM_OK && fvm_object_define_method_rooted(c,p.victim,"onEnterFrame",child_call,&p,0)==FVM_OK);
 assert(fvm_movieclip_advance(c,p.root,1)==FVM_OK && p.count==3 && p.order[0]==0 && p.order[1]==1 && p.order[2]==3);tick_clean(c);assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
