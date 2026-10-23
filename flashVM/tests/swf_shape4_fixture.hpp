/* Byte-authored SWF 8 fill-only shapes; independent of the production encoder. */
#ifndef FVM_SHAPE4_FIXTURE_HPP
#define FVM_SHAPE4_FIXTURE_HPP
#define FVM_GRADIENT_FIXTURE_ONLY
#include "swf_gradient.cpp"
static inline Bytes shape4(unsigned int kind = 0x10, unsigned int spread = 0,
                           unsigned int interpolation = 0, int focal = 0,
                           unsigned int stops = 2, size_t *header = 0,
                           size_t *flags_at = 0, size_t *lines_at = 0)
{
    Bytes b; u16(b, 9); Bits rect; rect.put(16, 5);
    rect.put(0, 16); rect.put(320, 16); rect.put(0, 16); rect.put(320, 16);
    append(b, rect.b); append(b, rect.b);
    if (flags_at) *flags_at = b.size(); b.push_back(0);
    b.push_back(1); b.push_back((unsigned char)kind); Bits matrix;
    matrix.put(1, 1); matrix.put(11, 5); matrix.put(320, 11); matrix.put(320, 11);
    matrix.put(0, 1); matrix.put(16, 5); matrix.put(160, 16); matrix.put(160, 16);
    append(b, matrix.b); if (header) *header = b.size();
    b.push_back((unsigned char)((spread << 6) | (interpolation << 4) | stops));
    for (unsigned int i = 0; i < stops; ++i) {
        unsigned int ratio = stops == 1 ? 0 : i * 255 / (stops - 1);
        b.push_back((unsigned char)ratio); b.push_back((unsigned char)(255 - ratio));
        b.push_back(0); b.push_back((unsigned char)ratio); b.push_back(255);
    }
    if (kind == 0x13) u16(b, (unsigned int)focal & 0xffff);
    if (lines_at) *lines_at = b.size(); b.push_back(0);
    Bits records; records.put(1, 4); records.put(0, 4);
    records.put(0, 1); records.put(3, 5); records.put(16, 5);
    records.put(0, 16); records.put(0, 16); records.put(1, 1);
    edge(records, 320, 0); edge(records, 0, 320); edge(records, -320, 0); edge(records, 0, -320);
    records.put(0, 6); append(b, records.b); return b;
}
static inline Bytes shape4_tags(const Bytes &payload)
{
    Bytes tags, empty; tag(tags, 83, payload); tag(tags, 26, place(6, 1, 9, 0, 0));
    tag(tags, 1, empty); tag(tags, 0, empty); return tags;
}
/* Compare the file-to-graph renderer with the public synchronous sampler. */
static inline void shape4_pixels(FVM_Context *c, FVM_Object *root, unsigned int kind,
                                unsigned int spread, unsigned int interpolation, double focal,
                                unsigned int stop_count = 2)
{
    unsigned char actual[1024], expected[1024]; std::memset(actual, 0, sizeof(actual));
    std::memset(expected, 0, sizeof(expected)); FVM_RasterSurface a = {actual,16,16,64}, b = {expected,16,16,64};
    FVM_Result rendered = fvm_movieclip_render(c, root, &a); if (rendered == FVM_UNSUPPORTED) return;
    assert(rendered == FVM_OK); FVM_Path *path = 0; assert(fvm_path_create(&path) == FVM_OK);
    FVM_PathCommand cmd; std::memset(&cmd, 0, sizeof(cmd)); cmd.verb = FVM_PATH_MOVE;
    assert(fvm_path_add(path, &cmd) == FVM_OK); cmd.verb = FVM_PATH_LINE;
    cmd.x[0]=16; assert(fvm_path_add(path,&cmd)==FVM_OK); cmd.y[0]=16;
    assert(fvm_path_add(path,&cmd)==FVM_OK); cmd.x[0]=0; assert(fvm_path_add(path,&cmd)==FVM_OK);
    cmd.verb=FVM_PATH_CLOSE; assert(fvm_path_add(path,&cmd)==FVM_OK);
    FVM_Gradient g; assert(fvm_gradient_init(&g)==FVM_OK); FVM_GradientStop stops[15];
    for (unsigned int i=0;i<stop_count;++i) { unsigned int ratio=stop_count==1?0:i*255/(stop_count-1);
        stops[i].ratio=ratio/255.0; stops[i].color.r=(unsigned char)(255-ratio);
        stops[i].color.g=0; stops[i].color.b=(unsigned char)ratio; stops[i].color.a=255; }
    g.stops=stops; g.count=stop_count; g.type=kind==0x10?FVM_GRADIENT_LINEAR:FVM_GRADIENT_RADIAL;
    g.matrix.xx=g.matrix.yy=4; g.matrix.x0=g.matrix.y0=8;
    assert(fvm_path_draw_gradient_focal(path,&b,&g,1,spread,interpolation,focal)==FVM_OK);
    assert(std::memcmp(actual,expected,sizeof(actual))==0); fvm_path_destroy(path);
}
#endif
