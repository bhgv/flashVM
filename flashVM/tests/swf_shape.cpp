#define FVM_SCENE_FIXTURE_ONLY
#include "swf_scene.cpp"
#include "../src/swf/shape.hpp"
#include "../src/core/runtime.hpp"
static inline void move(Bits &bits, long x, long y, bool style = false)
{
    bits.put(0, 1); bits.put(style ? 11 : 1, 5); bits.put(16, 5);
    bits.put((unsigned long)x, 16); bits.put((unsigned long)y, 16);
    if (style) { bits.put(1, 1); bits.put(1, 1); }
}
static inline void edge(Bits &bits, long x, long y)
{
    bits.put(1, 1); bits.put(1, 1); bits.put(9, 4); bits.put(1, 1);
    bits.put((unsigned long)x, 11); bits.put((unsigned long)y, 11);
}
static inline void square(Bits &bits, long x, long y, long size, bool style = false)
{
    move(bits, x, y, style); edge(bits, size, 0); edge(bits, 0, size); edge(bits, -size, 0); edge(bits, 0, -size);
}
static inline Bytes shape(unsigned int id, unsigned int version, bool hole = false, bool curve = false, bool new_styles = false)
{
    Bytes b; u16(b, id); Bits bounds; bounds.put(16, 5); bounds.put(0, 16); bounds.put(320, 16);
    bounds.put((unsigned long)(curve ? -160 : 0), 16); bounds.put(320, 16); append(b, bounds.b);
    b.push_back(1); b.push_back(0); b.push_back(255); b.push_back(0); b.push_back(0); if (version == 32) b.push_back(255);
    b.push_back(1); u16(b, 20); b.push_back(0); b.push_back(0); b.push_back(0); if (version == 32) b.push_back(255);
    Bits records; records.put(1, 4); records.put(1, 4);
    if (curve) {
        move(records, 0, 0, true); records.put(1, 1); records.put(0, 1); records.put(9, 4);
        records.put(80, 11); records.put((unsigned long)-160, 11); records.put(80, 11); records.put(160, 11);
        edge(records, 0, 160); edge(records, -160, 0); edge(records, 0, -160);
    } else square(records, 0, 0, 320, true);
    if (hole) square(records, 80, 80, 160);
    if (new_styles) {
        records.put(0, 1); records.put(16, 5); records.bit = 0;
        records.b.push_back(1); records.b.push_back(0); records.b.push_back(0); records.b.push_back(0); records.b.push_back(255);
        if (version == 32) records.b.push_back(255);
        records.b.push_back(0); records.put(1, 4); records.put(0, 4);
        records.put(0, 1); records.put(11, 5); records.put(16, 5); records.put(200, 16); records.put(0, 16);
        records.put(1, 1); square(records, 200, 0, 120);
    }
    records.put(0, 6); append(b, records.b); return b;
}
static inline FVM_Object *display_node(FVM_Object *root)
{
    FVM_Object *children = root->resource_result.object->properties["children"].object;
    return children->indexed[1].object;
}
static inline void pixels(FVM_Context *c, FVM_Object *root, bool hole = false, bool blue = false)
{
    unsigned char image[24 * 16 * 4]; std::memset(image, 0, sizeof(image));
    FVM_RasterSurface surface; surface.pixels = image; surface.width = 24; surface.height = 16; surface.stride = 96;
    FVM_Result r = fvm_movieclip_render(c, root, &surface);
    if (r == FVM_UNSUPPORTED) return;
    assert(r == FVM_OK);
    unsigned char *red = image + 4 * (2 * 24 + 2), *center = image + 4 * (8 * 24 + 8);
    if (red[0] < 250 || red[1] || red[2] || red[3] < 250) std::fprintf(stderr, "shape pixel hole=%d blue=%d rgba=%u,%u,%u,%u\n", hole, blue, red[0], red[1], red[2], red[3]);
    assert(red[0] >= 250 && red[1] == 0 && red[2] == 0 && red[3] >= 250);
    if (hole) assert(center[3] == 0); else assert(center[0] >= 250 && center[3] >= 250);
    if (blue) { unsigned char *p = image + 4 * (3 * 24 + 13); assert(p[2] >= 250 && p[0] == 0 && p[3] >= 250); }
}
#ifndef FVM_SHAPE_FIXTURE_ONLY
int main()
{
    FVM_SWFShape decoded; Bytes source = shape(9, 32, true); assert(fvm_swf_read_shape(source.begin(), source.size(), 32, decoded) == FVM_OK);
    assert(decoded.id == 9 && decoded.paints.size() == 2); assert(decoded.paints[0].commands.size() == 12);
    assert(decoded.paints[0].commands[0].verb == FVM_PATH_MOVE && decoded.paints[0].commands[5].verb == FVM_PATH_CLOSE);
    for (size_t count = 0; count < source.size(); ++count) {
        decoded.id = 777; assert(fvm_swf_read_shape(source.begin(), count, 32, decoded) == FVM_INVALID_ARGUMENT); assert(decoded.id == 777);
    }
    Bytes extra = source; extra.push_back(0); assert(fvm_swf_read_shape(extra.begin(), extra.size(), 32, decoded) == FVM_INVALID_ARGUMENT);
    FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0, *other = 0;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_context_create(rt, &other) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
    Bytes empty;
    for (unsigned int version = 0; version < 3; ++version) {
        unsigned int code = version == 0 ? 2 : version == 1 ? 22 : 32; Bytes tags;
        tag(tags, code, shape(9, code, true)); tag(tags, 26, place(2 | 4 | 32, 1, 9, 0, 0, "hiddenShape"));
        tag(tags, 1, empty); tag(tags, 0, empty); FVM_Object *root = load(c, tags, 1); tick(c, root); pixels(c, root, true);
        assert(get(c, root, "hiddenShape").type == FVM_VALUE_UNDEFINED);
        FVM_Value a, out; fvm_value_number(&a, -16383);
        assert(fvm_object_call(c, root, "getInstanceAtDepth", 1, &a, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
        FVM_Object *node = display_node(root); assert(node && node->prototype == 0);
        assert(fvm_gc_collect(c, 0) == FVM_OK); pixels(c, root, true);
        fvm_value_undefined(&a); a.type = FVM_VALUE_OBJECT; a.object = root;
        FVM_Channel *ch = 0; assert(fvm_channel_create(&ch) == FVM_OK);
        assert(fvm_channel_send_value(c, ch, &a, 0) == FVM_OK); assert(fvm_channel_receive_value(other, ch, &out, 0) == FVM_OK);
        FVM_Object *copy = out.object; assert(fvm_gc_pin(other, copy) == FVM_OK); assert(fvm_gc_collect(other, 0) == FVM_OK); pixels(other, copy, true);
        fvm_channel_destroy(ch);
    }
    Bytes tags; tag(tags, 32, shape(9, 32, false, false, true)); tag(tags, 26, place(2 | 4, 1, 9, 0, 0));
    tag(tags, 1, empty); tag(tags, 0, empty); FVM_Object *root = load(c, tags, 1); tick(c, root); pixels(c, root, false, true);
    tags.clear(); tag(tags, 32, shape(9, 32, false, true)); tag(tags, 26, place(2 | 4, 1, 9, 0, 0));
    tag(tags, 1, empty); tag(tags, 0, empty); root = load(c, tags, 1); tick(c, root);
    FVM_Value out; assert(fvm_object_call(c, root, "getRect", 0, 0, &out) == FVM_OK);
    num(c, out.object, "yMin", -4); num(c, out.object, "xMax", 8);
    tags.clear(); tag(tags, 32, shape(9, 32)); tag(tags, 32, shape(10, 32, true));
    tag(tags, 26, place(2 | 4, 1, 9, 0, 0)); tag(tags, 1, empty);
    tag(tags, 26, place(1 | 2, 1, 10, 0, 0)); tag(tags, 1, empty); tag(tags, 0, empty);
    root = load(c, tags, 2); tick(c, root); FVM_Object *node = display_node(root); pixels(c, root);
    tick(c, root); assert(display_node(root) == node); pixels(c, root, true);
    FVM_Object *jump = load(c, tags, 2); seek(c, jump, 2); pixels(c, jump, true); seek(c, jump, 1); pixels(c, jump);
    tags.clear(); tag(tags, 32, shape(9, 32)); tag(tags, 32, shape(10, 32, true));
    Bytes exports; u16(exports, 1); u16(exports, 9); text(exports, "Art"); tag(tags, 56, exports);
    tag(tags, 26, place(2 | 4, 1, 9, 0, 0)); tag(tags, 26, place(2 | 4, 2, 9, 400, 0)); tag(tags, 1, empty);
    tag(tags, 26, place(1 | 2, 1, 10, 0, 0)); tag(tags, 1, empty); tag(tags, 0, empty);
    root = load(c, tags, 2); tick(c, root);
    FVM_Object *children = root->resource_result.object->properties["children"].object;
    FVM_Object *first = children->indexed[1].object, *second = children->indexed[2].object;
    FVM_Object *original_paths = second->resource_result.object->properties["paths"].object;
    assert(first->resource_result.object->properties["paths"].object == original_paths);
    tick(c, root); assert(first->resource_result.object->properties["paths"].object != original_paths);
    assert(second->resource_result.object->properties["paths"].object == original_paths);
    FVM_Value attach_args[3]; fvm_value_string(attach_args, "Art"); fvm_value_string(attach_args + 1, "invalidClip"); fvm_value_number(attach_args + 2, 5);
    assert(fvm_object_call(c, root, "attachMovie", 3, attach_args, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
    FVM_Value root_value; fvm_value_undefined(&root_value); root_value.type = FVM_VALUE_OBJECT; root_value.object = root;
    unsigned char *snapshot = 0; size_t snapshot_size = 0; assert(fvm_value_serialize(c, &root_value, &snapshot, &snapshot_size) == FVM_OK);
    assert(fvm_value_deserialize(other, snapshot, snapshot_size, &out) == FVM_OK); fvm_serialized_free(snapshot);
    FVM_Object *restored = out.object; assert(fvm_gc_pin(other, restored) == FVM_OK); assert(fvm_gc_collect(other, 0) == FVM_OK);
    FVM_Object *restored_children = restored->resource_result.object->properties["children"].object;
    assert(restored_children->indexed[1].object->resource_result.object->properties["paths"].object !=
           restored_children->indexed[2].object->resource_result.object->properties["paths"].object);
    FVM_Channel *file = 0; FVM_Result opened = fvm_channel_open_file("flashvm-shape-test.bin", "w", &file);
    if (opened == FVM_OK) {
        assert(fvm_channel_send_value(c, file, &root_value, 0) == FVM_OK); fvm_channel_destroy(file);
        assert(fvm_channel_open_file("flashvm-shape-test.bin", "r", &file) == FVM_OK);
        assert(fvm_channel_receive_value(other, file, &out, 0) == FVM_OK); fvm_channel_destroy(file); assert(std::remove("flashvm-shape-test.bin") == 0);
        restored = out.object; assert(fvm_gc_pin(other, restored) == FVM_OK); assert(fvm_gc_collect(other, 0) == FVM_OK);
        assert(display_node(restored)->resource_result.object->properties["static_art"].number == 1);
    } else assert(opened == FVM_UNSUPPORTED);
    tags.clear(); tag(tags, 32, shape(7, 32)); definition(tags); tag(tags, 1, empty); tag(tags, 0, empty);
    Bytes swf = movie(tags, 1); FVM_Object *bad = root;
    assert(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "bad", &bad) == FVM_INVALID_ARGUMENT && !bad);
    fvm_context_destroy(other); fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
#endif
