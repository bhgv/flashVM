/* One call graph preserves receiver/argument aliases and cycles; returned functions stay live. */
#define FVM_REMOTE_FUNCTION_FIXTURE_ONLY
#include "function_remote.cpp"
#ifdef OS_PORTS
struct GraphState {FVM_Context *source;FVM_Object *returned;unsigned calls;};
static FVM_Result graph_call(FVM_Context *c,FVM_Object *receiver,unsigned argc,const FVM_Value *argv,FVM_Value *out,void *raw)
{
 GraphState *state=(GraphState *)raw;assert(c==state->source);++state->calls;
 if(!argc){*out=object(state->returned);return FVM_OK;}
 assert(argc==2 && argv[0].type==FVM_VALUE_OBJECT && argv[0].object==argv[1].object && receiver==argv[0].object && receiver->owner==c);
 FVM_Value self;assert(fvm_object_get(c,receiver,"self",&self)==FVM_OK && self.object==receiver);*out=argv[0];return FVM_OK;
}
#endif
int main()
{
#ifdef OS_PORTS
 FVM_Config cfg;FVM_Runtime *runtime=0;FVM_Context *source=0,*client=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&runtime)==FVM_OK && fvm_context_create(runtime,&source)==FVM_OK && fvm_context_create(runtime,&client)==FVM_OK);
 FVM_ResourceBroker *broker=0;assert(fvm_broker_start("127.0.0.1","127.0.0.1",0,&broker)==FVM_OK && fvm_context_set_broker(source,broker)==FVM_OK && fvm_context_function_transfer(source,1)==FVM_OK);
 Original native;native.context=source;native.calls=0;native.bias=300;assert(fvm_object_create(source,&native.capture)==FVM_OK);GraphState state;state.source=source;state.calls=0;assert(fvm_function_create_native(source,original,&native,native.capture,&state.returned)==FVM_OK);
 FVM_Object *entry=0;assert(fvm_function_create_native(source,graph_call,&state,state.returned,&entry)==FVM_OK);char *uri=0;assert(fvm_broker_export(source,entry,&uri)==FVM_OK);FVM_Object *proxy=0;assert(fvm_broker_import(client,uri,&proxy)==FVM_OK && fvm_gc_pin(client,proxy)==FVM_OK && fvm_gc_collect(source,0)==FVM_OK);
 FVM_Object *cycle=0;assert(fvm_object_create(client,&cycle)==FVM_OK);FVM_Value args[2],out;args[0]=args[1]=object(cycle);assert(fvm_object_set(client,cycle,"self",args)==FVM_OK && fvm_gc_pin(client,cycle)==FVM_OK);
 assert(fvm_context_function_serve(source,1)==FVM_OK && fvm_function_invoke(client,proxy,cycle,2,args,&out)==FVM_OK && out.type==FVM_VALUE_OBJECT && out.object!=cycle);
 FVM_Value self;assert(fvm_object_get(client,out.object,"self",&self)==FVM_OK && self.object==out.object);
 /* A returned unregistered C function becomes another interface on the same listener. */
 assert(fvm_function_invoke(client,proxy,0,0,0,&out)==FVM_OK && out.type==FVM_VALUE_OBJECT && fvm_function_is_proxy(out.object) && fvm_gc_pin(client,out.object)==FVM_OK);FVM_Object *returned=out.object;
 FVM_Value arg;fvm_value_number(&arg,7);assert(fvm_gc_collect(client,0)==FVM_OK && fvm_function_invoke(client,returned,0,1,&arg,&out)==FVM_OK && out.number==307);
 assert(fvm_context_function_serve(source,0)==FVM_OK && state.calls==2 && native.calls==1 && fvm_gc_collect(source,0)==FVM_OK);
 /* Reusing the interface after host ownership is reclaimed must not enter the heap. */
 assert(fvm_function_invoke(client,returned,0,0,0,&out)==FVM_NOT_FOUND && native.calls==1);
 fvm_context_destroy(source);fvm_context_destroy(client);fvm_serialized_free(uri);fvm_broker_stop(broker);fvm_broker_release(broker);fvm_runtime_destroy(runtime);
#endif
}
