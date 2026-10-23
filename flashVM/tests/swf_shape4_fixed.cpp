#include "swf_shape4_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
    /* Every representable interior FIXED8 value, including both signed extremes. */
    for(int focal=-255;focal<=255;++focal) {
        unsigned int spread=(unsigned int)(focal+255)%3, interpolation=(unsigned int)(focal+255)%2;
        Bytes bytes=shape4(0x13,spread,interpolation,focal); FVM_SWFShape decoded;
        assert(fvm_swf_read_shape(bytes.begin(),bytes.size(),83,decoded)==FVM_OK);
        assert(decoded.paints[0].gradient_focal==focal/256.0);
        FVM_Object *root=load(c,shape4_tags(bytes),1); tick(c,root);
        shape4_pixels(c,root,0x13,spread,interpolation,focal/256.0);
        assert(fvm_gc_unpin(c,root)==FVM_OK); assert(fvm_gc_collect(c,0)==FVM_OK);
    }
    fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
