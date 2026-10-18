/* Revoke, detach and stop without allocating while mixed target entries retain other heaps. */
#define FVM_REMOTE_FUNCTION_ALLOCATION_FIXTURE_ONLY
#include "function_remote_export_allocation.cpp"
int main()
{
#ifdef OS_PORTS
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *source[2],*client=0;FVM_ResourceBroker *broker=0;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&client)==FVM_OK && fvm_broker_start("127.0.0.1","127.0.0.1",0,&broker)==FVM_OK);
 Original state[2];FVM_Object *fn[2];char *uri[2],*renewed[2];FVM_Channel *channel=0;char *channel_uri=0;
 for(unsigned i=0;i<2;++i){assert(fvm_context_create(rt,source+i)==FVM_OK && fvm_context_set_broker(source[i],broker)==FVM_OK && fvm_context_function_transfer(source[i],1)==FVM_OK);state[i].context=source[i];state[i].calls=0;state[i].bias=i?200:100;assert(fvm_object_create(source[i],&state[i].capture)==FVM_OK && fvm_function_create_native(source[i],original,state+i,state[i].capture,fn+i)==FVM_OK && fvm_gc_pin(source[i],fn[i])==FVM_OK);}
 /* Keep a resource before two function entries: removal must move a retained function token. */
 FVM_Object *wrapper=0;assert(fvm_channel_create(&channel)==FVM_OK && fvm_extensions_wrap_channel(source[0],channel,&wrapper)==FVM_OK && fvm_broker_export(source[0],wrapper,&channel_uri)==FVM_OK);
 for(unsigned i=0;i<2;++i)assert(fvm_broker_export(source[i],fn[i],uri+i)==FVM_OK && fn[i]->gc_pins==2);
 FVM_Object *old_proxy=0,*new_proxy=0;assert(fvm_broker_import(client,uri[0],&old_proxy)==FVM_OK && fvm_gc_pin(client,old_proxy)==FVM_OK);
 assert(fvm_context_function_serve(source[0],1)==FVM_OK);FVM_Value arg,out;fvm_value_number(&arg,7);assert(fvm_function_invoke(client,old_proxy,0,1,&arg,&out)==FVM_OK && out.number==107);
 allowance=0;FVM_Result result=fvm_context_function_revoke(source[0],fn[0]);allowance=-1;assert(result==FVM_ERROR && fvm_context_function_serve(source[0],0)==FVM_OK);
 allowance=0;result=fvm_context_function_revoke(source[0],fn[0]);allowance=-1;assert(result==FVM_OK && fn[0]->gc_pins==1 && fn[1]->gc_pins==2);
 allowance=0;result=fvm_context_function_revoke(source[0],fn[0]);allowance=-1;assert(result==FVM_NOT_FOUND);
 FVM_BrokerStats stats;assert(fvm_broker_stats(broker,&stats)==FVM_OK && stats.exported_resources==2);
 assert(fvm_broker_export(source[0],fn[0],renewed)==FVM_OK && std::strcmp(uri[0],renewed[0]) && fn[0]->gc_pins==2 && fvm_broker_import(client,renewed[0],&new_proxy)==FVM_OK && fvm_gc_pin(client,new_proxy)==FVM_OK);
 assert(fvm_context_function_serve(source[0],1)==FVM_OK && fvm_function_invoke(client,old_proxy,0,1,&arg,&out)==FVM_NOT_FOUND && fvm_function_invoke(client,new_proxy,0,1,&arg,&out)==FVM_OK && out.number==107 && fvm_context_function_serve(source[0],0)==FVM_OK);
 /* Restore entry order so detachment, then stop, must each move another owner's function. */
 allowance=0;result=fvm_context_function_revoke(source[1],fn[1]);allowance=-1;assert(result==FVM_OK && fn[1]->gc_pins==1);
 assert(fvm_broker_export(source[1],fn[1],renewed+1)==FVM_OK && std::strcmp(uri[1],renewed[1]) && fn[1]->gc_pins==2);
 allowance=0;result=fvm_context_set_broker(source[0],0);allowance=-1;assert(result==FVM_OK && fn[0]->gc_pins==1 && fn[1]->gc_pins==2 && state[0].calls==2);
 assert(fvm_broker_stats(broker,&stats)==FVM_OK && stats.exported_resources==2 && fvm_function_invoke(client,new_proxy,0,1,&arg,&out)==FVM_NOT_FOUND);
 allowance=0;fvm_broker_stop(broker);allowance=-1;assert(fn[1]->gc_pins==2);
 allowance=0;result=fvm_context_set_broker(source[1],0);allowance=-1;assert(result==FVM_OK && fn[1]->gc_pins==1);
 for(unsigned i=0;i<2;++i){assert(fvm_gc_collect(source[i],0)==FVM_OK);fvm_context_destroy(source[i]);fvm_serialized_free(uri[i]);fvm_serialized_free(renewed[i]);}
 fvm_serialized_free(channel_uri);fvm_channel_release(channel);fvm_context_destroy(client);fvm_broker_release(broker);fvm_runtime_destroy(rt);
#else
 assert(fvm_context_function_revoke(0,0)==FVM_UNSUPPORTED);
#endif
}
