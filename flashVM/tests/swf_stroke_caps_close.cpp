#include "swf_stroke_caps_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
    for(unsigned type=0;type<4;++type) for(unsigned cap=0;cap<3;++cap) for(unsigned join=0;join<2;++join)
    for(unsigned topology=0;topology<4;++topology) {
        bool closed=(topology&1)!=0,no_close=(topology&2)!=0;
        FVM_Object *root=load(c,bs_tags(caps_shape(type,cap,join,80,closed,no_close)),1); tick(c,root);
        caps_pixels(c,root,type,cap,join,80,closed,no_close); assert(fvm_gc_unpin(c,root)==FVM_OK); assert(fvm_gc_collect(c,0)==FVM_OK);
    }
    fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
