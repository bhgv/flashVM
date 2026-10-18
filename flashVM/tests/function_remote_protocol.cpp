/* A valid serialized result cannot bypass target/source/context/request correlation checks. */
#define FVM_REMOTE_FUNCTION_FIXTURE_ONLY
#include "function_remote.cpp"
#include "../ports/network_port.h"
#include "../ports/thread_port.h"
#include "../src/capi/function_rpc_constants.h"
#include <cstdio>
#ifdef OS_PORTS
struct Reply {FVM_PortSocket *listener;unsigned char *payload;size_t size;unsigned alter;};
static bool rpc_bytes(FVM_PortSocket *socket,void *raw,size_t size,bool send)
{unsigned char *p=(unsigned char *)raw;while(size){long n=send?fvm_port_socket_send(socket,p,size):fvm_port_socket_recv(socket,p,size);if(n<=0)return false;p+=n;size-=(size_t)n;}return true;}
static void rpc_u32(unsigned char *p,unsigned long n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
static void reply(void *raw)
{
 Reply *state=(Reply *)raw;FVM_PortSocket *socket=fvm_port_tcp_accept(state->listener,2000);assert(socket);unsigned char h[144],response[116],buffer[256];assert(rpc_bytes(socket,h,sizeof(h),false));
 size_t size=((size_t)h[108]<<24)|((size_t)h[109]<<16)|((size_t)h[110]<<8)|h[111];while(size){size_t n=size<sizeof(buffer)?size:sizeof(buffer);assert(rpc_bytes(socket,buffer,n,false));size-=n;}
 std::memset(response,0,sizeof(response));std::memcpy(response,"FVMR",4);response[4]=2;response[5]=10;response[6]=9;
 std::memcpy(response+8,h+40,32);std::memcpy(response+40,h+72,32);std::memcpy(response+72,h+104,4);std::memcpy(response+76,h+112,32);rpc_u32(response+112,(unsigned long)state->size);
 const unsigned offsets[]={0,8,40,72,76};if(state->alter && state->alter<5)response[offsets[state->alter]]^=1;
 if(state->alter==5 || state->alter==6)rpc_u32(response+112,0);
 if(state->alter==6)rpc_u32(response+108,(unsigned long)(-FVM_EXCEPTION));
 if(state->alter==7)response[7]=1; /* Successful replies cannot publish pending faults. */
 if(state->alter==8)rpc_u32(response+108,99);
 if(state->alter==9 || state->alter==10){response[7]=1;rpc_u32(response+108,(unsigned long)(-FVM_ERROR));}
 if(state->alter==11)response[7]=2; /* Unknown flag bits must not reach value decoding. */
 rpc_bytes(socket,response,sizeof(response),true);
 if(state->alter==9 || state->alter==10){unsigned char fault[FVM_FUNCTION_RPC_FAULT_SIZE];std::memset(fault,0,sizeof(fault));if(state->alter==10){std::memcpy(fault,"FVMF\0\0\0\1",8);std::memset(fault+64,'x',128);}rpc_bytes(socket,fault,sizeof(fault),true);}
 if(state->alter!=5 && state->alter!=6)rpc_bytes(socket,state->payload,state->size,true);fvm_port_socket_close(socket);
}
#endif
int main()
{
#ifdef OS_PORTS
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);FVM_Value value,out;fvm_value_number(&value,37);Reply state;assert(fvm_value_serialize(c,&value,&state.payload,&state.size)==FVM_OK);
 for(state.alter=0;state.alter<12;++state.alter){unsigned short port=0;state.listener=fvm_port_tcp_listen("127.0.0.1",0,&port);assert(state.listener);FVM_PortThread *thread=fvm_port_thread_start(reply,&state);assert(thread);char uri[256];std::sprintf(uri,"fvm://127.0.0.1:%u/00000000000000000000000000000000/11111111111111111111111111111111/9",(unsigned)port);FVM_Object *proxy=0;assert(fvm_broker_import(c,uri,&proxy)==FVM_OK && fvm_gc_pin(c,proxy)==FVM_OK);
  FVM_Result result=fvm_function_invoke(c,proxy,0,0,0,&out);if(!state.alter)assert(result==FVM_OK && out.number==37);else{FVM_ExceptionInfo fault;assert(result==FVM_EXCEPTION && fvm_exception_info(c,&fault)==FVM_OK && fault.code==FVM_RESOURCE_DISCONNECTED && fvm_exception_clear(c)==FVM_OK);}
  fvm_port_thread_join(thread);fvm_port_socket_close(state.listener);assert(!c->gc_frames && !c->gc_native_depth);
 }
 fvm_serialized_free(state.payload);fvm_context_destroy(c);fvm_runtime_destroy(rt);
#endif
}
