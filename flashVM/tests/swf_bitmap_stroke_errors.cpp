#include "swf_bitmap_stroke_fixture.hpp"
int main()
{
    FVM_SWFShape out; Bytes source=bs_shape();
    for(size_t n=0;n<source.size();++n) { out.id=777; assert(fvm_swf_read_shape(source.begin(),n,83,out)==FVM_INVALID_ARGUMENT); assert(out.id==777); }
    Bytes zero=bs_shape(0x42,80,false,false,0,0); assert(fvm_swf_read_shape(zero.begin(),zero.size(),83,out)==FVM_INVALID_ARGUMENT);
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
    Bytes tags,empty; tag(tags,83,source); tag(tags,1,empty); tag(tags,0,empty); Bytes bytes=movie(tags,1); FVM_Object *root=0;
    assert(fvm_movieclip_load_swf(c,bytes.begin(),bytes.size(),"root",&root)==FVM_INVALID_ARGUMENT && !root);
    bytes=movie(bs_tags(bs_shape(0x42,80,false,false,0,8)),1);
    assert(fvm_movieclip_load_swf(c,bytes.begin(),bytes.size(),"root",&root)==FVM_INVALID_ARGUMENT && !root);
    bytes=movie(bs_tags(source),1); bytes[3]=7;
    assert(fvm_movieclip_load_swf(c,bytes.begin(),bytes.size(),"root",&root)==FVM_INVALID_ARGUMENT && !root);
    /* Width admission is checked even without a graphics backend, before any destination write. */
    unsigned char rgba[4]={255,0,0,255},pixels[1024],before[1024]; FVM_RasterSurface bitmap={rgba,1,1,4},dst={pixels,16,16,64};
    FVM_BitmapPaint paint; assert(fvm_bitmap_paint_init(&paint)==FVM_OK); paint.source=&bitmap; FVM_Path *path=bs_path();
    std::memset(pixels,71,sizeof(pixels)); std::memcpy(before,pixels,sizeof(pixels));
    const double widths[]={-1,4096.01,HUGE_VAL,-HUGE_VAL,HUGE_VAL-HUGE_VAL};
    for(unsigned i=0;i<5;++i) assert(fvm_path_bitmap_paint_quality(path,&dst,&paint,widths[i],0,true)==FVM_INVALID_ARGUMENT);
    assert(std::memcmp(pixels,before,sizeof(pixels))==0);
    /* Aliased source/destination must match a separately retained source snapshot. */
    unsigned char aliased[1024],snapshot[1024],expected[1024];
    for(unsigned i=0;i<256;++i) { aliased[i*4]=(unsigned char)i; aliased[i*4+1]=aliased[i*4+2]=0; aliased[i*4+3]=255; }
    std::memcpy(snapshot,aliased,sizeof(snapshot)); std::memcpy(expected,aliased,sizeof(expected));
    FVM_RasterSurface self={aliased,16,16,64},copy={snapshot,16,16,64},oracle={expected,16,16,64};
    paint.source=&copy; paint.matrix.x0=1;
    FVM_Result expected_status=fvm_path_bitmap_paint_quality(path,&oracle,&paint,4,0,true);
    paint.source=&self; assert(fvm_path_bitmap_paint_quality(path,&self,&paint,4,0,true)==expected_status);
    assert(expected_status==FVM_OK || expected_status==FVM_UNSUPPORTED);
    assert(std::memcmp(aliased,expected,sizeof(aliased))==0); fvm_path_destroy(path);
    fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
