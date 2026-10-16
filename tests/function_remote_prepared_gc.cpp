/* Remote invocation keeps the original prepared body and its active capture across self-unbind/GC. */
#define FVM_REMOTE_FUNCTION_FIXTURE_ONLY
#include "function_remote.cpp"
#ifdef OS_PORTS
struct Prepared {FVM_Context *context;FVM_Object *capture;unsigned calls;};
static FVM_Result prepared(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *raw)
{
 Prepared *state=(Prepared *)raw;assert(c==state->context && frame->capture==state->capture && frame->argc==1 && frame->receiver==frame->argv[0].object);++state->calls;
 assert(fvm_function_set_body(c,frame->function,0,0,0)==FVM_OK && fvm_gc_collect(c,0)==FVM_OK);
 FVM_Value value;assert(fvm_object_get(c,frame->capture,"marker",&value)==FVM_OK && value.number==17);
 assert(fvm_object_get(c,frame->argv[0].object,"self",&value)==FVM_OK && value.object==frame->argv[0].object);
 *out=object(frame->capture);return FVM_OK;
}
#endif
int main()
{
#ifdef OS_PORTS
 FVM_Config cfg;FVM_Runtime *runtime=0;FVM_Context *source=0,*client=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&runtime)==FVM_OK && fvm_context_create(runtime,&source)==FVM_OK && fvm_context_create(runtime,&client)==FVM_OK);
 FVM_ResourceBroker *broker=0;assert(fvm_broker_start("127.0.0.1","127.0.0.1",0,&broker)==FVM_OK && fvm_context_set_broker(source,broker)==FVM_OK && fvm_context_function_transfer(source,1)==FVM_OK);
 const unsigned char code[]={0x96,5,0,7,44,0,0,0,0x3e,0};FVM_Object *function=0;assert(fvm_function_create_avm1(source,code,sizeof(code),0,0,0,&function)==FVM_OK);
 Prepared state;state.context=source;state.calls=0;assert(fvm_object_create(source,&state.capture)==FVM_OK);FVM_Value value;fvm_value_number(&value,17);assert(fvm_object_set(source,state.capture,"marker",&value)==FVM_OK && fvm_function_set_body(source,function,prepared,&state,state.capture)==FVM_OK);
 char *uri=0;FVM_Object *proxy=0;assert(fvm_broker_export(source,function,&uri)==FVM_OK && fvm_broker_import(client,uri,&proxy)==FVM_OK && fvm_gc_pin(client,proxy)==FVM_OK && fvm_gc_collect(source,0)==FVM_OK);
 FVM_Object *cycle=0;assert(fvm_object_create(client,&cycle)==FVM_OK);FVM_Value arg=object(cycle),out;assert(fvm_object_set(client,cycle,"self",&arg)==FVM_OK && fvm_gc_pin(client,cycle)==FVM_OK);
 assert(fvm_context_function_serve(source,1)==FVM_OK && fvm_function_invoke(client,proxy,cycle,1,&arg,&out)==FVM_OK && out.type==FVM_VALUE_OBJECT);
 assert(fvm_object_get(client,out.object,"marker",&value)==FVM_OK && value.number==17);
 assert(fvm_function_invoke(client,proxy,0,0,0,&out)==FVM_OK && out.number==44 && fvm_context_function_serve(source,0)==FVM_OK && state.calls==1 && !source->gc_frames && !source->gc_native_depth);
 fvm_context_destroy(source);fvm_context_destroy(client);fvm_serialized_free(uri);fvm_broker_stop(broker);fvm_broker_release(broker);fvm_runtime_destroy(runtime);
#endif
}
