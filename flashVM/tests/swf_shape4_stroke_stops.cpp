#include "swf_shape4_stroke_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
    for(unsigned count=1;count<=15;++count) for(unsigned mode=0;mode<6;++mode) for(unsigned kind=0;kind<2;++kind) {
        unsigned type=kind?0x13:0x10; Bytes bytes=shape4_stroke(type,mode/2,mode%2,-128,80,count);
        FVM_Object *root=load(c,shape4_tags(bytes),1); tick(c,root);
        stroke4_pixels(c,root,type,mode/2,mode%2,kind?-.5:0,80,count);
        assert(fvm_gc_unpin(c,root)==FVM_OK); assert(fvm_gc_collect(c,0)==FVM_OK);
    }
    fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
