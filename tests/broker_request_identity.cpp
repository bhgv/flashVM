/* Raw wire identities are counted canonical hex; malformed IDs cannot create peers or invoke originals. */
#define FVM_REMOTE_FUNCTION_FIXTURE_ONLY
#include "function_remote.cpp"
#include "flashvm/mutex.h"
#include "../ports/network_port.h"
#include <cstdlib>
#ifdef OS_PORTS
static bool bytes(FVM_PortSocket *socket,void *raw,size_t count,bool send)
{unsigned char *p=(unsigned char *)raw;while(count){long n=send?fvm_port_socket_send(socket,p,count):fvm_port_socket_recv(socket,p,count);if(n<=0)return false;p+=n;count-=(size_t)n;}return true;}
static void u32(unsigned char *p,unsigned long n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
static unsigned long word(const unsigned char *p)
{return ((unsigned long)p[0]<<24)|((unsigned long)p[1]<<16)|((unsigned long)p[2]<<8)|p[3];}
static void header(unsigned char *h,const char *uri,unsigned version,unsigned operation,unsigned kind,char source,unsigned long context)
{const char *path=std::strchr(uri+6,'/');assert(path && std::strlen(path)>=68);std::memset(h,0,144);std::memcpy(h,"FVMC",4);h[4]=(unsigned char)version;h[5]=(unsigned char)operation;h[6]=(unsigned char)kind;std::memcpy(h+8,path+1,32);std::memcpy(h+40,path+34,32);std::memset(h+72,source,32);u32(h+104,context);std::memset(h+112,'b',32);}
static unsigned long lock_request(FVM_ResourceBroker *broker,const char *uri,unsigned operation,char source,unsigned long context,unsigned bad)
{
 unsigned char h[144],response[8],discard[2048];header(h,uri,1,operation,8,source,context);if(bad==1)h[88]=0;if(bad==2)h[103]='g';if(bad==3)u32(h+104,0);
 FVM_PortSocket *socket=fvm_port_tcp_connect("127.0.0.1",fvm_broker_port(broker));assert(socket && fvm_port_socket_timeout(socket,2000));assert(bytes(socket,h,112,true) && bytes(socket,response,8,false));size_t size=(size_t)word(response+4);assert(size<65536);while(size){size_t count=size<sizeof(discard)?size:sizeof(discard);assert(bytes(socket,discard,count,false));size-=count;}fvm_port_socket_close(socket);return word(response);
}
static unsigned long call_request(FVM_ResourceBroker *broker,const char *uri,const unsigned char *payload,size_t size,unsigned bad,FVM_Context *client)
{
 unsigned char h[144],response[116];header(h,uri,2,10,9,'a',1);u32(h+108,(unsigned long)size);if(bad==1)h[88]=0;if(bad==2)h[103]='g';if(bad==3)h[128]=0;if(bad==4)h[143]='g';if(bad==5)u32(h+104,0);
 FVM_PortSocket *socket=fvm_port_tcp_connect("127.0.0.1",fvm_broker_port(broker));assert(socket && fvm_port_socket_timeout(socket,2000));assert(bytes(socket,h,144,true) && bytes(socket,(void *)payload,size,true) && bytes(socket,response,116,false));assert(!std::memcmp(response,"FVMR",4) && response[7]==0 && !std::memcmp(response+8,h+40,32) && !std::memcmp(response+40,h+72,32) && !std::memcmp(response+72,h+104,4) && !std::memcmp(response+76,h+112,32));size_t count=(size_t)word(response+112);assert(count<65536);unsigned char *result=count?(unsigned char *)std::malloc(count):0;assert(!count || (result && bytes(socket,result,count,false)));fvm_port_socket_close(socket);if(!bad){FVM_Value value;assert(fvm_value_deserialize(client,result,count,&value)==FVM_OK && value.type==FVM_VALUE_NUMBER && value.number==100);}else assert(!count);std::free(result);return word(response+108);
}
#endif
int main()
{
#ifdef OS_PORTS
 FVM_Config cfg;FVM_Runtime *runtime=0;FVM_Context *source=0,*client=0;FVM_ResourceBroker *broker=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&runtime)==FVM_OK && fvm_context_create(runtime,&source)==FVM_OK && fvm_context_create(runtime,&client)==FVM_OK && fvm_broker_start("127.0.0.1","127.0.0.1",0,&broker)==FVM_OK && fvm_context_set_broker(source,broker)==FVM_OK);
 FVM_Mutex *mutex=0;FVM_Object *wrapper=0;char *mutex_uri=0;assert(fvm_mutex_create(source,&mutex)==FVM_OK && fvm_extensions_wrap_mutex(source,mutex,&wrapper)==FVM_OK && fvm_broker_export(source,wrapper,&mutex_uri)==FVM_OK);
 for(unsigned bad=1;bad<=3;++bad){assert(lock_request(broker,mutex_uri,1,'c',1,bad)!=0);FVM_BrokerStats stats;int held=1;assert(fvm_broker_stats(broker,&stats)==FVM_OK && !stats.peer_contexts && fvm_mutex_touch(mutex,&held)==FVM_OK && !held);}
 assert(lock_request(broker,mutex_uri,1,'c',1,0)==0 && lock_request(broker,mutex_uri,2,'c',2,0)!=0 && lock_request(broker,mutex_uri,2,'d',1,0)!=0);int held=0;assert(fvm_mutex_touch(mutex,&held)==FVM_OK && held && lock_request(broker,mutex_uri,2,'c',1,0)==0);
 assert(lock_request(broker,mutex_uri,1,'c',2,0)==0 && lock_request(broker,mutex_uri,2,'c',1,0)!=0 && lock_request(broker,mutex_uri,2,'c',2,0)==0);FVM_BrokerStats stats;assert(fvm_broker_stats(broker,&stats)==FVM_OK && stats.peer_contexts==3);
 Original state;state.context=source;state.bias=100;state.calls=0;FVM_Object *function=0;char *uri=0;assert(fvm_object_create(source,&state.capture)==FVM_OK && fvm_function_create_native(source,original,&state,state.capture,&function)==FVM_OK && fvm_context_function_transfer(source,1)==FVM_OK && fvm_broker_export(source,function,&uri)==FVM_OK);
 FVM_Object *frame=0;assert(fvm_object_create(client,&frame)==FVM_OK);FVM_Value value;fvm_value_undefined(&value);frame->indexed[0]=value;value=object(frame);unsigned char *payload=0;size_t size=0;assert(fvm_value_serialize(client,&value,&payload,&size)==FVM_OK && fvm_context_function_serve(source,1)==FVM_OK);
 assert(call_request(broker,uri,payload,size,0,client)==0);for(unsigned bad=1;bad<=5;++bad)assert(call_request(broker,uri,payload,size,bad,client)==(unsigned long)(-FVM_INVALID_ARGUMENT));assert(call_request(broker,uri,payload,size,0,client)==0 && fvm_context_function_serve(source,0)==FVM_OK && state.calls==2 && !source->gc_frames && !source->gc_native_depth);
 fvm_serialized_free(payload);fvm_serialized_free(uri);fvm_serialized_free(mutex_uri);fvm_broker_stop(broker);fvm_mutex_release(mutex);fvm_context_destroy(source);fvm_context_destroy(client);fvm_broker_release(broker);fvm_runtime_destroy(runtime);
#endif
}
