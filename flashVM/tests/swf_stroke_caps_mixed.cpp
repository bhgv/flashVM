#include "swf_stroke_caps_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
    for(unsigned cap=0;cap<3;++cap) for(unsigned join=0;join<2;++join) {
        size_t end=0,style=0; Bytes stroke=caps_shape(0,cap,join,80,true,false,&end),unused=bs_shape(0x42,80,false,false,0,7,&style),bytes; (void)unused;
        for(size_t i=0;i<style-2;++i) bytes.push_back(stroke[i]); bytes.push_back(1);
        bytes.push_back(0); bytes.push_back(0); bytes.push_back(0); bytes.push_back(255); bytes.push_back(96); bytes.push_back(1);
        for(size_t i=style;i<end;++i) bytes.push_back(stroke[i]);
        Bits records; records.put(1,4); records.put(1,4); move(records,0,0,true);
        edge(records,320,0); edge(records,0,320); edge(records,-320,0); edge(records,0,-320); records.put(0,6); append(bytes,records.b);
        FVM_Object *root=load(c,bs_tags(bytes),1); tick(c,root);
        unsigned char actual[1024],expected[1024]; std::memset(actual,0,sizeof(actual)); std::memset(expected,0,sizeof(expected)); FVM_RasterSurface a={actual,16,16,64},b={expected,16,16,64};
        FVM_Result r=fvm_movieclip_render(c,root,&a); if(r==FVM_UNSUPPORTED) continue; assert(r==FVM_OK);
        FVM_Path *path=0; assert(fvm_path_create(&path)==FVM_OK); FVM_PathCommand cmd; std::memset(&cmd,0,sizeof(cmd)); cmd.verb=FVM_PATH_MOVE;
        assert(fvm_path_add(path,&cmd)==FVM_OK); cmd.verb=FVM_PATH_LINE; cmd.x[0]=16; assert(fvm_path_add(path,&cmd)==FVM_OK);
        cmd.y[0]=16; assert(fvm_path_add(path,&cmd)==FVM_OK); cmd.x[0]=0; assert(fvm_path_add(path,&cmd)==FVM_OK); cmd.y[0]=0; assert(fvm_path_add(path,&cmd)==FVM_OK);
        cmd.verb=FVM_PATH_CLOSE; assert(fvm_path_add(path,&cmd)==FVM_OK); FVM_RasterColor blue={0,0,255,96},red={255,0,0,128};
        assert(fvm_path_draw(path,&b,blue,0,1)==FVM_OK); assert(fvm_path_draw_quality(path,&b,red,4,0,true,cap,join)==FVM_OK);
        assert(std::memcmp(actual,expected,sizeof(actual))==0); fvm_path_destroy(path);
    }
    fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
