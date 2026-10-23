#include "swf_stroke_caps_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
    for(unsigned cap=0;cap<3;++cap) {
        FVM_Object *root=load(c,bs_tags(caps_shape(0,cap,0)),1); tick(c,root);
        unsigned char pixels[1024]; std::memset(pixels,0,sizeof(pixels)); FVM_RasterSurface s={pixels,16,16,64};
        FVM_Result r=fvm_movieclip_render(c,root,&s); if(r==FVM_UNSUPPORTED) continue; assert(r==FVM_OK);
        assert(pixels[4*(8*16+8)+3]==128);
        if(cap==1) { for(unsigned y=0;y<16;++y) for(unsigned x=0;x<16;++x) assert(pixels[4*(y*16+x)+3]==((x>=2 && x<14 && y>=6 && y<10)?128:0)); }
        if(cap==2) { for(unsigned y=0;y<16;++y) for(unsigned x=0;x<16;++x) assert(pixels[4*(y*16+x)+3]==((y>=6 && y<10)?128:0)); }
        if(cap==0) assert(pixels[4*(6*16)+3]>0 && pixels[4*(6*16)+3]<128);
    }
    for(unsigned join=0;join<2;++join) {
        FVM_Object *root=load(c,bs_tags(caps_shape(0,0,join,80,true,false)),1); tick(c,root);
        unsigned char pixels[1024]; std::memset(pixels,0,sizeof(pixels)); FVM_RasterSurface s={pixels,16,16,64};
        FVM_Result r=fvm_movieclip_render(c,root,&s); if(r==FVM_UNSUPPORTED) continue; assert(r==FVM_OK);
        unsigned alpha=pixels[4*(2*16+13)+3]; assert(join?alpha==0:alpha>0);
    }
    fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
