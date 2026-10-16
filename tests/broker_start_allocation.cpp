/* C authority startup must contain constructor OOM, preserve status and release failed listeners. */
#define FVM_REMOTE_FUNCTION_ALLOCATION_FIXTURE_ONLY
#include "function_remote_export_allocation.cpp"
int main()
{
#ifdef OS_PORTS
 bool succeeded=false;unsigned failures=0;FVM_ResourceBroker *broker=0;
 for(long budget=0;budget<100 && !succeeded;++budget){broker=(FVM_ResourceBroker *)1;FVM_Result result=FVM_ERROR;
  try{allowance=budget;result=fvm_broker_start("127.0.0.1","127.0.0.1",0,&broker);allowance=-1;}
  catch(...){allowance=-1;assert(!"C++ exception escaped broker startup C API");}
  if(result==FVM_OK){succeeded=true;assert(broker && fvm_broker_port(broker));}
  else{++failures;assert(result==FVM_OUT_OF_MEMORY && !broker);}
 }
 assert(succeeded && failures>=3);
 unsigned short port=fvm_broker_port(broker);FVM_ResourceBroker *duplicate=(FVM_ResourceBroker *)1;
 assert(fvm_broker_start("127.0.0.1","127.0.0.1",port,&duplicate)==FVM_ERROR && !duplicate);
 FVM_BrokerStats stats;assert(fvm_broker_stats(broker,&stats)==FVM_OK && !stats.peer_contexts && !stats.exported_resources);
 fvm_broker_stop(broker);fvm_broker_release(broker);broker=0;
 assert(fvm_broker_start("127.0.0.1","127.0.0.1",port,&broker)==FVM_OK && fvm_broker_port(broker)==port);
 fvm_broker_stop(broker);fvm_broker_release(broker);
#endif
}
