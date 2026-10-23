#include "swf_shape4_stroke_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
    for(unsigned mode=0;mode<6;++mode) {
        size_t fill_lines=0,stroke_flags=0,stroke_lines=0;
        Bytes fill=shape4(0x13,mode/2,mode%2,128,2,0,0,&fill_lines);
        Bytes stroke=shape4(0x13,2-mode/2,1-mode%2,-128,2,0,&stroke_flags,&stroke_lines),bytes;
        for(size_t i=0;i<fill_lines;++i) bytes.push_back(fill[i]);
        bytes.push_back(1); u16(bytes,80); bytes.push_back(8); bytes.push_back(0);
        for(size_t i=stroke_flags+2;i<stroke_lines;++i) bytes.push_back(stroke[i]);
        Bits records; records.put(1,4); records.put(1,4); move(records,0,0,true);
        edge(records,320,0); edge(records,0,320); edge(records,-320,0); edge(records,0,-320); records.put(0,6); append(bytes,records.b);
        FVM_Object *root=load(c,shape4_tags(bytes),1); tick(c,root);
        unsigned char actual[1024],expected[1024]; std::memset(actual,0,sizeof(actual)); std::memset(expected,0,sizeof(expected));
        FVM_RasterSurface a={actual,16,16,64},b={expected,16,16,64}; FVM_Result r=fvm_movieclip_render(c,root,&a);
        if(r==FVM_UNSUPPORTED) continue; assert(r==FVM_OK);
        FVM_Path *path=0; assert(fvm_path_create(&path)==FVM_OK); FVM_PathCommand cmd; std::memset(&cmd,0,sizeof(cmd)); cmd.verb=FVM_PATH_MOVE;
        assert(fvm_path_add(path,&cmd)==FVM_OK); cmd.verb=FVM_PATH_LINE; cmd.x[0]=16; assert(fvm_path_add(path,&cmd)==FVM_OK);
        cmd.y[0]=16; assert(fvm_path_add(path,&cmd)==FVM_OK); cmd.x[0]=0; assert(fvm_path_add(path,&cmd)==FVM_OK);
        cmd.y[0]=0; assert(fvm_path_add(path,&cmd)==FVM_OK); cmd.verb=FVM_PATH_CLOSE; assert(fvm_path_add(path,&cmd)==FVM_OK);
        FVM_Gradient g; FVM_GradientStop stops[2]; assert(fvm_gradient_init(&g)==FVM_OK); std::memset(stops,0,sizeof(stops));
        stops[0].color.r=stops[0].color.a=255; stops[1].ratio=1; stops[1].color.b=stops[1].color.a=255;
        g.type=FVM_GRADIENT_RADIAL; g.matrix.xx=g.matrix.yy=4; g.matrix.x0=g.matrix.y0=8; g.stops=stops; g.count=2;
        assert(fvm_path_draw_gradient_focal(path,&b,&g,1,mode/2,mode%2,.5)==FVM_OK);
        assert(fvm_path_stroke_gradient_focal(path,&b,&g,4,2-mode/2,1-mode%2,-.5)==FVM_OK);
        assert(std::memcmp(actual,expected,sizeof(actual))==0); fvm_path_destroy(path);
        assert(fvm_gc_collect(c,0)==FVM_OK);
    }
    fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
