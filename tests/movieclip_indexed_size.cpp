/* Indexed width/height reads use local display bounds, including children and singular ancestors. */
#define main indexed_size_path_fixture_main
#include "movieclip_paths.cpp"
#undef main
#include "flashvm/execution.h"
#include "flashvm/affine.h"
#include <cmath>
static FVM_Value size_object(FVM_Object *o)
{ FVM_Value v; fvm_value_undefined(&v); v.type = FVM_VALUE_OBJECT; v.object = o; return v; }
static void size_scalar(FVM_Context *c, FVM_Object *o, const char *key, double n)
{ FVM_Value v; fvm_value_number(&v, n); assert(fvm_object_set(c, o, key, &v) == FVM_OK); }
static void size_point(FVM_Context *c, FVM_Object *o, const char *method, double x, double y)
{ FVM_Value a[2], v; fvm_value_number(a, x); fvm_value_number(a + 1, y); assert(fvm_object_call(c, o, method, 2, a, &v) == FVM_OK); }
static void size_rect(FVM_Context *c, FVM_Object *o, double x, double y, double w, double h)
{
    FVM_Value a, out; fvm_value_number(&a, 0xff0000); assert(fvm_object_call(c, o, "beginFill", 1, &a, &out) == FVM_OK);
    size_point(c, o, "moveTo", x, y); size_point(c, o, "lineTo", x + w, y);
    size_point(c, o, "lineTo", x + w, y + h); size_point(c, o, "lineTo", x, y + h); size_point(c, o, "lineTo", x, y);
    assert(fvm_object_call(c, o, "endFill", 0, 0, &out) == FVM_OK);
}
static void size_expect(FVM_Context *c, FVM_Object *o, unsigned index, double expected)
{
    FVM_Value target = size_object(o), at, out; fvm_value_number(&at, index); fvm_value_number(&out, -999);
    assert(fvm_exec_display_property(c, 0, &target, &at, 0, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && std::fabs(out.number - expected) < 1e-7);
}
#ifndef FVM_INDEXED_SIZE_FIXTURE_ONLY
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
        if (version) assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK);
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *root = 0; assert(fvm_movieclip_create(c, "root", 1, &root) == FVM_OK && fvm_gc_pin(c, root) == FVM_OK);
        FVM_Object *clip = child(c, root, "clip", 1); size_expect(c, clip, 8, 0); size_expect(c, clip, 9, 0);
        size_rect(c, clip, 0, 0, 10, 20); size_expect(c, clip, 8, 10); size_expect(c, clip, 9, 20);
        size_scalar(c, clip, "_x", 17); size_scalar(c, clip, "_y", -31);
        size_scalar(c, clip, "_xscale", -200); size_scalar(c, clip, "_yscale", 300);
        size_expect(c, clip, 8, 20); size_expect(c, clip, 9, 60);
        size_scalar(c, clip, "_rotation", 90); size_expect(c, clip, 8, 60); size_expect(c, clip, 9, 20);
        size_scalar(c, root, "_xscale", 0); size_scalar(c, root, "_yscale", 0);
        size_expect(c, clip, 8, 60); size_expect(c, clip, 9, 20);
        size_scalar(c, clip, "_xscale", 100); size_scalar(c, clip, "_yscale", 100); size_scalar(c, clip, "_rotation", 0);
        FVM_Object *nested = child(c, clip, "nested", 2); size_rect(c, nested, 0, 0, 5, 5);
        size_scalar(c, nested, "_x", 30); size_scalar(c, nested, "_y", -10);
        size_expect(c, clip, 8, 35); size_expect(c, clip, 9, 30);
        FVM_Affine shear = {1, 0, 0.5, 1, 7, 8}; assert(fvm_movieclip_set_matrix(c, clip, &shear) == FVM_OK);
        size_expect(c, clip, 8, 32.5); size_expect(c, clip, 9, 30);
        FVM_Value target = size_object(clip), at; fvm_value_number(&at, 8);
        assert(fvm_exec_display_property(c, 0, &target, &at, 0, &target) == FVM_OK && std::fabs(target.number - 32.5) < 1e-7);
        FVM_Value value, out; fvm_value_number(&value, 5); fvm_value_number(&out, 77);
        target = size_object(clip); assert(fvm_exec_display_property(c, 1, &target, &at, &value, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
        assert(fvm_gc_collect(c, 0) == FVM_OK);
        assert(c->gc_frames == 0 && c->gc_native_depth == 0); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
#endif
