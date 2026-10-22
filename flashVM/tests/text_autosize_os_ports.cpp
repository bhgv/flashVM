/* Independent owner-thread contexts repeatedly fit text and collect GC without shared VM state. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include "textfield_counted_text.cpp"
#include "../ports/thread_port.h"
#ifdef FVM_HAS_FONTS
struct AutoJob { unsigned passes;bool failed; };
/* Each worker owns its runtime, context, font registration and rooted field. */
static void auto_worker(void *raw)
{
 AutoJob *job=(AutoJob*)raw;FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;
 if(fvm_config_init(&cfg)!=FVM_OK || fvm_runtime_create(&cfg,&rt)!=FVM_OK || fvm_context_create(rt,&c)!=FVM_OK){job->failed=true;fvm_runtime_destroy(rt);return;}
 FVM_Object *field=counted_field(c),*format=0;FVM_Value value,arg,out;
 assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK && fvm_gc_pin(c,format)==FVM_OK);fvm_value_number(&value,40);assert(fvm_object_set(c,format,"size",&value)==FVM_OK);
 fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;assert(fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_OK);
 fvm_value_boolean(&value,1);assert(fvm_object_set(c,field,"autoSize",&value)==FVM_OK);
 for(unsigned i=0;i<64;++i){
  assert(fvm_value_string(&value,i%2?"AVA":"AV")==FVM_OK);
  if(fvm_object_set(c,field,"text",&value)!=FVM_OK || counted_metric(c,field,"_width")!=(i%2?76:52) || counted_metric(c,field,"_height")!=48 || fvm_gc_collect(c,0)!=FVM_OK){job->failed=true;break;}++job->passes;
 }
 fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
#endif
/* Join all worker-owned contexts before reading their completion counters. */
int main()
{
#ifdef FVM_HAS_FONTS
 AutoJob jobs[4];FVM_PortThread *threads[4];for(unsigned i=0;i<4;++i){jobs[i].passes=0;jobs[i].failed=false;threads[i]=fvm_port_thread_start(auto_worker,jobs+i);assert(threads[i]);}
 for(unsigned i=0;i<4;++i){fvm_port_thread_join(threads[i]);assert(!jobs[i].failed && jobs[i].passes==64);}
#endif
 return 0;
}
