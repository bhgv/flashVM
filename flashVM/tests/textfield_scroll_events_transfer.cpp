/* Binary/channel transfers retain native registrations and script listeners after sender teardown. */
#include "textfield_scroll_events_fixture.hpp"
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#ifdef FVM_HAS_FONTS
static FVM_Object *script_listener(FVM_Context *c)
{
    FVM_Object *object=0,*fn=0;assert(fvm_object_create(c,&object)==FVM_OK);FVM_Value v;fvm_value_number(&v,0);assert(fvm_object_set(c,object,"seen",&v)==FVM_OK);
    Program p;p.variable("this");p.text("seen");p.variable("this");p.text("seen");p.op(0x4e);p.number(1);p.op(0x47);p.op(0x4f);
    p.variable("this");p.text("source");p.variable("arguments");p.number(0);p.op(0x4e);p.op(0x4f);p.number(0);p.finish();
    assert(fvm_function_create_avm1(c,p.bytes.begin(),p.bytes.size(),0,0,0,&fn)==FVM_OK);v=class_value(fn);assert(fvm_object_set(c,object,"onScroller",&v)==FVM_OK);return object;
}
#endif
int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0,*d=0,*e=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_context_create(rt,&d)==FVM_OK && fvm_context_create(rt,&e)==FVM_OK);
    FVM_Object *field=scroll_field(c),*listener=script_listener(c);assert(scroll_listen(c,field,listener));FVM_Value value=class_value(listener);assert(fvm_object_set(c,field,"observer",&value)==FVM_OK);
    FVM_Value root=class_value(field),out;unsigned char *wire=0;size_t size=0;
    assert(fvm_value_serialize(c,&root,&wire,&size)==FVM_OK && fvm_standard_text_install(d)==FVM_OK && fvm_value_deserialize(d,wire,size,&out)==FVM_OK && fvm_gc_pin(d,out.object)==FVM_OK);FVM_Object *copy=out.object;fvm_serialized_free(wire);
    FVM_Channel *channel=0;assert(fvm_channel_create(&channel)==FVM_OK && fvm_channel_send_value(c,channel,&root,0)==FVM_OK);fvm_context_destroy(c);
    assert(fvm_standard_text_install(e)==FVM_OK && fvm_channel_receive_value(e,channel,&out,0)==FVM_OK && fvm_gc_pin(e,out.object)==FVM_OK);fvm_channel_destroy(channel);
    FVM_Context *contexts[]={d,e};FVM_Object *fields[]={copy,out.object};
    for(unsigned j=0;j<2;++j)
    {
        assert(fvm_gc_collect(contexts[j],0)==FVM_OK);scroll_number(contexts[j],fields[j],"scroll",2);
        assert(fvm_object_get(contexts[j],fields[j],"observer",&value)==FVM_OK && value.type==FVM_VALUE_OBJECT);FVM_Object *observer=value.object;
        assert(counted_metric(contexts[j],observer,"seen")==1 && fvm_object_get(contexts[j],observer,"source",&value)==FVM_OK && value.object==fields[j]);
        assert(scroll_listen(contexts[j],fields[j],observer,"removeListener"));scroll_number(contexts[j],fields[j],"scroll",1);assert(counted_metric(contexts[j],observer,"seen")==1);
        assert(!contexts[j]->gc_frames && !contexts[j]->gc_native_depth && !contexts[j]->avm1_execution_depth);
    }
    fvm_context_destroy(d);fvm_context_destroy(e);fvm_runtime_destroy(rt);
#endif
    return 0;
}
