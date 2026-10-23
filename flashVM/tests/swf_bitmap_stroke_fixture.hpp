/* Byte-authored LINESTYLE2 bitmap paints and separately composed sampling/coverage oracles. */
#ifndef FVM_BITMAP_STROKE_FIXTURE_HPP
#define FVM_BITMAP_STROKE_FIXTURE_HPP
#define FVM_BITMAP_SWF_FIXTURE_ONLY
#include "swf_bitmap_fill.cpp"
#include "../src/render/quality_internal.hpp"
static Bytes bs_raw()
{
    const unsigned char raw[]={128,128,0,0,255,0,255,0,0,0,0,0,64,0,0,64}; Bytes b;
    for(unsigned i=0;i<sizeof(raw);++i) b.push_back(raw[i]); return b;
}
static Bytes bs_shape(unsigned kind=0x42,unsigned width=80,bool no_close=false,bool closed=false,
                      int shift=0,unsigned bitmap_id=7,size_t *style_at=0,size_t *style_end=0)
{
    Bytes b; u16(b,9); Bits rect; rect.put(16,5); rect.put(0,16); rect.put(320,16); rect.put(0,16); rect.put(320,16);
    append(b,rect.b); append(b,rect.b); b.push_back(1); b.push_back(0); b.push_back(1);
    if(style_at) *style_at=b.size(); u16(b,width); b.push_back(8); b.push_back(no_close?4:0);
    b.push_back((unsigned char)kind); u16(b,bitmap_id); Bits m;
    m.put(1,1); m.put(22,5); m.put(20*65536,22); m.put(20*65536,22); m.put(0,1);
    m.put(16,5); m.put((unsigned long)shift,16); m.put((unsigned long)shift,16); append(b,m.b);
    if(style_end) *style_end=b.size();
    Bits records; records.put(0,4); records.put(1,4); records.put(0,1); records.put(9,5); records.put(16,5);
    records.put(closed?80:40,16); records.put(closed?80:160,16); records.put(1,1);
    if(closed) { edge(records,160,0); edge(records,0,160); edge(records,-160,-160); }
    else edge(records,240,0); records.put(0,6); append(b,records.b); return b;
}
static Bytes bs_tags(const Bytes &shape_bytes)
{
    Bytes tags,empty; tag(tags,36,lossless(5,2,2,bs_raw())); tag(tags,83,shape_bytes);
    tag(tags,26,place(6,1,9,0,0)); tag(tags,1,empty); tag(tags,0,empty); return tags;
}
static FVM_Path *bs_path(bool closed=false,bool no_close=false)
{
    FVM_Path *path=0; assert(fvm_path_create(&path)==FVM_OK); FVM_PathCommand cmd; std::memset(&cmd,0,sizeof(cmd));
    cmd.verb=FVM_PATH_MOVE; cmd.x[0]=closed?4:2; cmd.y[0]=closed?4:8; assert(fvm_path_add(path,&cmd)==FVM_OK);
    cmd.verb=FVM_PATH_LINE; cmd.x[0]=closed?12:14; assert(fvm_path_add(path,&cmd)==FVM_OK);
    if(closed) { cmd.y[0]=12; assert(fvm_path_add(path,&cmd)==FVM_OK); cmd.x[0]=cmd.y[0]=4;
        assert(fvm_path_add(path,&cmd)==FVM_OK); if(!no_close) { cmd.verb=FVM_PATH_CLOSE; assert(fvm_path_add(path,&cmd)==FVM_OK); } }
    return path;
}
static void bs_expected(unsigned char *out,unsigned kind,unsigned width=80,bool no_close=false,
                        bool closed=false,int shift=0,bool antialias=true,unsigned cap=0,unsigned join=0)
{
    unsigned char sampled[1024],mask[1024]; const unsigned char rgba[]={255,0,0,128,0,255,0,255,0,0,0,0,0,0,255,64};
    std::memset(out,0,1024); std::memset(sampled,0,sizeof(sampled)); std::memset(mask,0,sizeof(mask));
    FVM_RasterSurface source={(unsigned char*)rgba,2,2,8},dst={sampled,16,16,64},coverage={mask,16,16,64};
    FVM_Path *rectangle=0; assert(fvm_path_create(&rectangle)==FVM_OK); FVM_PathCommand cmd; std::memset(&cmd,0,sizeof(cmd));
    cmd.verb=FVM_PATH_MOVE; assert(fvm_path_add(rectangle,&cmd)==FVM_OK); cmd.verb=FVM_PATH_LINE; cmd.x[0]=16;
    assert(fvm_path_add(rectangle,&cmd)==FVM_OK); cmd.y[0]=16; assert(fvm_path_add(rectangle,&cmd)==FVM_OK);
    cmd.x[0]=0; assert(fvm_path_add(rectangle,&cmd)==FVM_OK); cmd.verb=FVM_PATH_CLOSE; assert(fvm_path_add(rectangle,&cmd)==FVM_OK);
    FVM_BitmapPaint paint; assert(fvm_bitmap_paint_init(&paint)==FVM_OK); paint.source=&source;
    paint.repeat=(kind==0x40 || kind==0x42); paint.smoothing=kind<0x42; paint.matrix.x0=paint.matrix.y0=shift/20.0;
    assert(fvm_path_draw_bitmap(rectangle,&dst,&paint,0)==FVM_OK); fvm_path_destroy(rectangle);
    FVM_Path *path=bs_path(closed,no_close); FVM_RasterColor white={255,255,255,255};
    assert(fvm_path_draw_quality(path,&coverage,white,width?width/20.0:1,0,antialias,cap,join)==FVM_OK); fvm_path_destroy(path);
    for(unsigned i=0;i<256;++i) { unsigned alpha=(sampled[i*4+3]*mask[i*4+3]+127)/255;
        if(alpha) { out[i*4]=sampled[i*4]; out[i*4+1]=sampled[i*4+1]; out[i*4+2]=sampled[i*4+2]; out[i*4+3]=(unsigned char)alpha; } }
}
static void bs_pixels(FVM_Context *c,FVM_Object *root,unsigned kind,unsigned width=80,
                      bool no_close=false,bool closed=false,int shift=0,bool antialias=true)
{
    unsigned char actual[1024],expected[1024]; std::memset(actual,0,sizeof(actual)); FVM_RasterSurface surface={actual,16,16,64};
    FVM_Result r=fvm_movieclip_render(c,root,&surface); if(r==FVM_UNSUPPORTED) return; assert(r==FVM_OK);
    bs_expected(expected,kind,width,no_close,closed,shift,antialias); assert(std::memcmp(actual,expected,sizeof(actual))==0);
}
#endif
