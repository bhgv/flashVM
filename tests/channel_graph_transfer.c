#include "flashvm/transfer.h"
#include "flashvm/gc.h"
#include "flashvm/table.h"
#include "flashvm/metamethod.h"
#include <assert.h>
#include <string.h>
static FVM_Result native(FVM_Context *c,FVM_Object *s,unsigned int n,const FVM_Value *a,FVM_Value *out,void *u) {
    (void)c;(void)s;(void)n;(void)a;(void)u;fvm_value_undefined(out);return FVM_OK;
}
int main(void) {
    FVM_Config cfg;FVM_Runtime *rt;FVM_Context *src,*dst;FVM_Channel *ch;
    FVM_Object *a,*b,*m;FVM_Value root,v,copy,edge,other;unsigned long freed;size_t pending;
    assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&src)==FVM_OK);assert(fvm_context_create(rt,&dst)==FVM_OK);
    assert(fvm_channel_create(&ch)==FVM_OK);assert(fvm_channel_set_capacity(ch,1)==FVM_OK);
    assert(fvm_object_create(src,&a)==FVM_OK);assert(fvm_object_create(src,&b)==FVM_OK);
    fvm_value_undefined(&root);root.type=FVM_VALUE_OBJECT;root.object=a;
    v=root;v.object=b;assert(fvm_object_set(src,a,"child",&v)==FVM_OK);
    assert(fvm_table_set_index(src,a,7,&v)==FVM_OK);
    assert(fvm_object_set(src,b,"parent",&root)==FVM_OK);
    assert(fvm_value_string(&v,"snapshot")==FVM_OK);assert(fvm_object_set(src,b,"label",&v)==FVM_OK);
    assert(fvm_object_set_prototype(src,a,b)==FVM_OK);
    assert(fvm_channel_send_value(src,ch,&root,0)==FVM_OK);
    assert(fvm_channel_send_value(src,ch,&root,0)==FVM_NOT_FOUND);
    assert(fvm_channel_try_receive(ch,&v)==FVM_UNSUPPORTED);
    assert(fvm_channel_pending(ch,&pending)==FVM_OK && pending==1);
    assert(fvm_value_string(&v,"changed")==FVM_OK);assert(fvm_object_set(src,b,"label",&v)==FVM_OK);
    assert(fvm_gc_collect(src,&freed)==FVM_OK && freed==2);
    fvm_context_destroy(src); /* Queued snapshot owns no source VM pointers. */
    assert(fvm_channel_receive_value(dst,ch,&copy,0)==FVM_OK);
    assert(fvm_gc_pin(dst,copy.object)==FVM_OK);
    assert(fvm_object_get(dst,copy.object,"child",&edge)==FVM_OK);
    assert(fvm_table_get_index(dst,copy.object,7,&other)==FVM_OK && other.object==edge.object);
    assert(fvm_object_get(dst,edge.object,"parent",&other)==FVM_OK && other.object==copy.object);
    assert(fvm_object_get(dst,edge.object,"label",&v)==FVM_OK && strcmp(v.string,"snapshot")==0);
    assert(fvm_object_get_prototype(dst,copy.object,&b)==FVM_OK && b==edge.object);
    assert(fvm_gc_collect(dst,&freed)==FVM_OK && freed==0);
    assert(fvm_gc_unpin(dst,copy.object)==FVM_OK);assert(fvm_gc_collect(dst,&freed)==FVM_OK && freed==2);
    assert(fvm_object_create(dst,&a)==FVM_OK);assert(fvm_object_create(dst,&m)==FVM_OK);
    assert(fvm_meta_register(dst,m,"__gc",native,0)==FVM_OK);
    assert(fvm_table_set_metatable(dst,a,m)==FVM_OK);
    root.object=a;assert(fvm_channel_send_value(dst,ch,&root,0)==FVM_UNSUPPORTED);
    assert(fvm_channel_pending(ch,&pending)==FVM_OK && pending==0);
    fvm_value_number(&v,42);assert(fvm_channel_send_value(dst,ch,&v,0)==FVM_OK);
    fvm_channel_close(ch);assert(fvm_channel_receive_value(dst,ch,&v,1)==FVM_OK && v.number==42);
    assert(fvm_channel_receive_value(dst,ch,&v,0)==FVM_ERROR);
    fvm_context_destroy(dst);fvm_channel_release(ch);fvm_runtime_destroy(rt);return 0;
}
