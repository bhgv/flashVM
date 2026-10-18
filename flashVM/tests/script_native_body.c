#include "flashvm/function.h"
#include "flashvm/execution.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "flashvm/standard.h"
#include "flashvm/transfer.h"
#include "flashvm/channel.h"
#include <assert.h>
static unsigned called;
static int mode;
static FVM_Result body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *raw)
{
    FVM_Value value,delta; unsigned long freed;
    assert(frame->struct_size==sizeof(*frame) && frame->abi_version==FVM_SCRIPT_FRAME_ABI);
    assert(frame->argc==1 && frame->argv[0].number==40 && frame->receiver==raw && frame->implicit_this==raw);
    assert(frame->arguments && frame->activation && frame->register_count==4 && frame->registers);
    assert(fvm_object_get(c,frame->activation,"x",&value)==FVM_OK && value.number==40);
    assert(frame->capture && fvm_object_get(c,frame->capture,"alive",&value)==FVM_OK && value.number==42);
    if(mode==1) assert(fvm_function_set_body(c,frame->function,0,0,0)==FVM_OK);
    assert(fvm_gc_collect(c,&freed)==FVM_OK);
    assert(fvm_object_get(c,frame->capture,"alive",&value)==FVM_OK && value.number==42);
    ++called;
    if(mode==2) { fvm_value_number(&value,123); return fvm_exception_raise(c,&value); }
    fvm_value_number(&delta,2);
    return fvm_exec_numeric(c,FVM_EXEC_ADD,&frame->argv[0],&delta,out);
}
int main(void)
{
    FVM_Config cfg; FVM_Runtime *rt; FVM_Context *c,*other; FVM_Object *fn,*receiver,*capture,*foreign;
    FVM_Value value,arg,sent,received; FVM_Channel *channel; const char *parameters[]={"x"};
    const unsigned char code[]={0x96,5,0,7,7,0,0,0,0x3e,0};
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK && fvm_context_create(rt,&other)==FVM_OK);
    assert(fvm_standard_install(c)==FVM_OK && fvm_gc_string_mode(c,FVM_GC_STRINGS_TRACED)==FVM_OK);
    assert(fvm_function_create_avm1(c,code,sizeof(code),1,parameters,0,&fn)==FVM_OK);
    assert(fvm_gc_pin(c,fn)==FVM_OK);
    assert(fvm_object_create(c,&receiver)==FVM_OK && fvm_gc_pin(c,receiver)==FVM_OK);
    assert(fvm_object_create(c,&capture)==FVM_OK); fvm_value_number(&value,42);
    assert(fvm_object_set(c,capture,"alive",&value)==FVM_OK);
    assert(fvm_object_create(other,&foreign)==FVM_OK);
    assert(fvm_function_set_body(c,fn,body,receiver,foreign)==FVM_INVALID_ARGUMENT);
    assert(fvm_function_set_body(c,receiver,body,receiver,capture)==FVM_INVALID_ARGUMENT);
    assert(fvm_function_set_body(c,fn,body,receiver,capture)==FVM_OK);
    assert(fvm_gc_collect(c,0)==FVM_OK); fvm_value_number(&arg,40);
    assert(fvm_function_invoke(c,fn,receiver,1,&arg,&value)==FVM_OK && value.number==42);
    mode=2; assert(fvm_function_invoke(c,fn,receiver,1,&arg,&value)==FVM_EXCEPTION);
    assert(fvm_exception_get(c,&value)==FVM_OK && value.number==123);
    assert(fvm_exception_clear(c)==FVM_OK);
    mode=0; assert(fvm_function_invoke(c,fn,receiver,1,&arg,&value)==FVM_OK && value.number==42);
    assert(fvm_channel_create(&channel)==FVM_OK); fvm_value_undefined(&sent); sent.type=FVM_VALUE_OBJECT; sent.object=fn;
    assert(fvm_channel_send_value(c,channel,&sent,0)==FVM_OK);
    assert(fvm_channel_receive_value(other,channel,&received,0)==FVM_OK && received.type==FVM_VALUE_OBJECT);
    assert(fvm_function_invoke(other,received.object,0,1,&arg,&value)==FVM_OK && value.number==7);
    fvm_channel_destroy(channel);
    mode=1; assert(fvm_function_invoke(c,fn,receiver,1,&arg,&value)==FVM_OK && value.number==42);
    assert(fvm_function_invoke(c,fn,receiver,1,&arg,&value)==FVM_OK && value.number==7);
    assert(called==4); fvm_context_destroy(other); fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
