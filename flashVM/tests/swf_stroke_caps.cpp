#include "swf_stroke_caps_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
    for(unsigned type=0;type<4;++type) for(unsigned cap=0;cap<3;++cap) for(unsigned join=0;join<2;++join) {
        Bytes b=caps_shape(type,cap,join); FVM_SWFShape decoded;
        assert(fvm_swf_read_shape(b.begin(),b.size(),83,decoded)==FVM_OK);
        assert(decoded.paints[0].stroke_cap==cap && decoded.paints[0].stroke_join==join);
        FVM_Object *root=load(c,bs_tags(b),1); tick(c,root); caps_pixels(c,root,type,cap,join);
    }
    fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
