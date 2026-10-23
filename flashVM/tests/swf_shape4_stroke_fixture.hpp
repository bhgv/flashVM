/* Raw LINESTYLE2 records and a public C89 stroke-sampler oracle. */
#ifndef FVM_SHAPE4_STROKE_FIXTURE_HPP
#define FVM_SHAPE4_STROKE_FIXTURE_HPP
#include "swf_shape4_fixture.hpp"
static inline Bytes shape4_stroke(unsigned kind=0x13,unsigned spread=0,unsigned interpolation=0,
                                  int focal=0,unsigned width=80,unsigned stops=2,
                                  unsigned flags0=8,unsigned flags1=0,bool closed=false,
                                  size_t *style_at=0)
{
    size_t flags=0,lines=0; Bytes fill=shape4(kind,spread,interpolation,focal,stops,0,&flags,&lines), b;
    for(size_t i=0;i<=flags;++i) b.push_back(fill[i]);
    b[flags]=1; b.push_back(0); b.push_back(1);
    if(style_at) *style_at=b.size();
    u16(b,width); b.push_back((unsigned char)flags0); b.push_back((unsigned char)flags1);
    if(flags0&8) {
        if(kind==0) { b.push_back(0); b.push_back(255); b.push_back(0); b.push_back(0); b.push_back(128); }
        else for(size_t i=flags+2;i<lines;++i) b.push_back(fill[i]);
    } else { b.push_back(255); b.push_back(0); b.push_back(0); b.push_back(128); }
    Bits records; records.put(0,4); records.put(1,4); records.put(0,1); records.put(9,5);
    records.put(16,5); records.put(closed?80:40,16); records.put(closed?80:160,16); records.put(1,1);
    if(closed) { edge(records,160,0); edge(records,0,160); edge(records,-160,-160); }
    else edge(records,240,0);
    records.put(0,6); append(b,records.b); return b;
}
static inline void stroke4_pixels(FVM_Context *c,FVM_Object *root,unsigned kind,unsigned spread,
                                 unsigned interpolation,double focal,unsigned width=80,unsigned count=2,
                                 bool closed=false,bool no_close=false)
{
    unsigned char actual[1024],expected[1024]; std::memset(actual,0,sizeof(actual)); std::memset(expected,0,sizeof(expected));
    FVM_RasterSurface a={actual,16,16,64},b={expected,16,16,64}; FVM_Result r=fvm_movieclip_render(c,root,&a);
    if(r==FVM_UNSUPPORTED) return;
    assert(r==FVM_OK);
    FVM_Path *path=0; assert(fvm_path_create(&path)==FVM_OK); FVM_PathCommand cmd; std::memset(&cmd,0,sizeof(cmd));
    cmd.verb=FVM_PATH_MOVE; cmd.x[0]=closed?4:2; cmd.y[0]=closed?4:8; assert(fvm_path_add(path,&cmd)==FVM_OK);
    cmd.verb=FVM_PATH_LINE; cmd.x[0]=closed?12:14; assert(fvm_path_add(path,&cmd)==FVM_OK);
    if(closed) { cmd.y[0]=12; assert(fvm_path_add(path,&cmd)==FVM_OK); cmd.x[0]=cmd.y[0]=4;
        assert(fvm_path_add(path,&cmd)==FVM_OK); if(!no_close) { cmd.verb=FVM_PATH_CLOSE; assert(fvm_path_add(path,&cmd)==FVM_OK); } }
    double w=width?width/20.0:1;
    if(kind==0) { FVM_RasterColor color={255,0,0,128}; assert(fvm_path_draw(path,&b,color,w,0)==FVM_OK); }
    else { FVM_Gradient g; FVM_GradientStop stops[15]; assert(fvm_gradient_init(&g)==FVM_OK);
        for(unsigned i=0;i<count;++i) { unsigned ratio=count==1?0:i*255/(count-1); stops[i].ratio=ratio/255.0;
            stops[i].color.r=(unsigned char)(255-ratio); stops[i].color.g=0; stops[i].color.b=(unsigned char)ratio; stops[i].color.a=255; }
        g.type=kind==0x10?FVM_GRADIENT_LINEAR:FVM_GRADIENT_RADIAL; g.matrix.xx=g.matrix.yy=4; g.matrix.x0=g.matrix.y0=8;
        g.stops=stops; g.count=count; assert(fvm_path_stroke_gradient_focal(path,&b,&g,w,spread,interpolation,focal)==FVM_OK); }
    if(std::memcmp(actual,expected,sizeof(actual))) {
        for(size_t i=0;i<sizeof(actual);++i) if(actual[i]!=expected[i]) {
            std::fprintf(stderr,"stroke4 kind=%u closed=%d no_close=%d byte=%lu actual=%u expected=%u\n",kind,closed,no_close,(unsigned long)i,actual[i],expected[i]); break;
        }
    }
    assert(std::memcmp(actual,expected,sizeof(actual))==0); fvm_path_destroy(path);
}
#endif
