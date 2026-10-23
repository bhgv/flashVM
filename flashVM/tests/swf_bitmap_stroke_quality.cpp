#include "swf_bitmap_stroke_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
    for(unsigned kind=0x40;kind<=0x43;++kind) for(unsigned quality=0;quality<4;++quality) {
        assert(fvm_stage_set_quality(c,(FVM_StageQuality)quality)==FVM_OK);
        FVM_Object *root=load(c,bs_tags(bs_shape(kind,80,true,true,10)),1); tick(c,root);
        bs_pixels(c,root,kind,80,true,true,10,quality!=FVM_STAGE_QUALITY_LOW);
    }
    fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
