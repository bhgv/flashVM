#include "swf_bitmap_stroke_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
    for(unsigned kind=0x40;kind<=0x43;++kind) {
        Bytes bytes=bs_shape(kind); FVM_SWFShape decoded; assert(fvm_swf_read_shape(bytes.begin(),bytes.size(),83,decoded)==FVM_OK);
        assert(decoded.paints.size()==1 && decoded.paints[0].stroke && decoded.paints[0].bitmap_type==kind);
        FVM_Object *root=load(c,bs_tags(bytes),1); tick(c,root); bs_pixels(c,root,kind);
        unsigned char pixels[1024]; std::memset(pixels,0,sizeof(pixels)); FVM_RasterSurface s={pixels,16,16,64};
        if(fvm_movieclip_render(c,root,&s)==FVM_OK) { const unsigned char *p=pixels+4*(8*16+4);
            assert(p[1]==0); if(kind==0x40 || kind==0x42) assert(p[0]==255 && p[2]==0 && p[3]==128);
            else assert(p[0]==0 && p[2]==255 && p[3]==64); }
    }
    fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
