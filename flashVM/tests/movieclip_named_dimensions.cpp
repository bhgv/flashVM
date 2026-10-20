/* Named dimensions and numeric actions must share geometry, coercion and transported state. */
#define FVM_AVM1_TARGET_FIXTURE_ONLY
#include "avm1_target_scope.cpp"
#include "flashvm/serialization.h"
#include "flashvm/execution_mode.h"
#include "flashvm/jit.h"
#include <cmath>
#include "../src/standard/movieclip_dimensions.hpp"
static unsigned conversions;
static bool fail_conversion;
/* A user conversion proves exactly-once evaluation and error propagation. */
static FVM_Result scalar(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out,
                         void *)
{
    ++conversions;
    fvm_value_number(out, 60);
    return fail_conversion ? FVM_ERROR : FVM_OK;
}
/* Draw an un-stroked rectangle, independent of any image decoder. */
static void rectangle(FVM_Context *c, FVM_Object *clip)
{
    FVM_Value args[2], out;
    fvm_value_number(args, 0xff0000);
    assert(fvm_object_call(c, clip, "beginFill", 1, args, &out) == FVM_OK);
    const double points[] = {0, 0, 20, 0, 20, 10, 0, 10, 0, 0};
    for (unsigned i = 0; i < 10; i += 2)
    {
        fvm_value_number(args, points[i]);
        fvm_value_number(args + 1, points[i + 1]);
        assert(fvm_object_call(c, clip, i ? "lineTo" : "moveTo", 2, args, &out) == FVM_OK);
    }
    assert(fvm_object_call(c, clip, "endFill", 0, 0, &out) == FVM_OK);
}
/* Read a numeric dimension through the public object API. */
static double dimension(FVM_Context *c, FVM_Object *clip, const char *key)
{
    FVM_Value out;
    assert(fvm_object_get(c, clip, key, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER);
    return out.number;
}
/* Store a copied scalar using the named setter. */
static void assign(FVM_Context *c, FVM_Object *clip, const char *key, double n)
{
    FVM_Value value;
    fvm_value_number(&value, n);
    assert(fvm_object_set(c, clip, key, &value) == FVM_OK);
}
/* The allocation sweep reuses geometry helpers without running this matrix. */
#ifndef FVM_NAMED_DIMENSIONS_FIXTURE_ONLY
int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 5; version <= 8; ++version)
        for (unsigned mode = 0; mode < 2; ++mode)
        {
            FVM_Context *c = 0;
            FVM_Object *root = 0;
            assert(fvm_context_create(rt, &c) == FVM_OK);
            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            swf[3] = (unsigned char)version;
            assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK &&
                   fvm_standard_install(c) == FVM_OK);
            assert(fvm_execution_set_mode(c, mode ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER,
                                          0) == FVM_OK);
            assert(fvm_movieclip_create(c, "root", 1, &root) == FVM_OK &&
                   fvm_gc_pin(c, root) == FVM_OK);
            FVM_Object *clip = child(c, root, "child", 1);
            rectangle(c, clip);
            assert(dimension(c, clip, "_width") == 20 && dimension(c, clip, "_height") == 10);
            assign(c, root, "_xscale", 300);
            assign(c, clip, "_x", 99);
            assert(dimension(c, clip, "_width") ==
                   20); /* Parent scale and translation do not alter width. */
            assign(c, clip, "_width", 40);
            assign(c, clip, "_height", 30);
            assert(dimension(c, clip, "_width") == 40 && dimension(c, clip, "_height") == 30);
            FVM_Value index, out, value, cv;
            fvm_value_undefined(&cv);
            cv.type = FVM_VALUE_OBJECT;
            cv.object = clip;
            fvm_value_number(&index, 8);
            assert(fvm_exec_display_property(c, 0, &cv, &index, 0, &out) == FVM_OK &&
                   out.number == 40);
            fvm_value_number(&value, 80);
            assert(fvm_exec_display_property(c, 1, &cv, &index, &value, &out) == FVM_OK);
            assert(dimension(c, clip, "_width") == 80);
            /* Actual GetMember/SetMember actions traverse the accessors in both execution modes. */
            assert(fvm_set_global(c, "sized", &cv) == FVM_OK);
            Bytes actions;
            global(actions, "sized");
            text(actions, "_height");
            integer(actions, 50);
            actions.push_back(0x4f);
            text(actions, "namedResult");
            global(actions, "sized");
            text(actions, "_height");
            actions.push_back(0x4e);
            actions.push_back(0x1d);
            assert(execute(c, root, actions) == FVM_OK);
            FVM_ExecutionStats stats;
            stats.struct_size = sizeof(stats);
            assert(fvm_execution_get_stats(c, &stats) == FVM_OK);
            if (mode && fvm_jit_available())
                assert(stats.native_entries > 0);
            else
                assert(stats.interpreter_entries > 0);
            number(c, root, "namedResult", 50);
            fvm_value_undefined(&value);
            assert(fvm_object_set(c, clip, "_width", &value) == FVM_OK);
            value.type = FVM_VALUE_NULL;
            assert(fvm_object_set(c, clip, "_width", &value) == FVM_OK);
            fvm_value_number(&value, std::numeric_limits<double>::quiet_NaN());
            assert(fvm_object_set(c, clip, "_width", &value) == FVM_OK);
            assert(dimension(c, clip, "_width") == 80);
            assign(c, clip, "_rotation", 90);
            assert(std::fabs(dimension(c, clip, "_width") - 50) < 0.000001);
            assert(std::fabs(dimension(c, clip, "_height") - 80) < 0.000001);
            assign(c, clip, "_rotation", 0);
            FVM_Object *coercer = 0;
            assert(fvm_object_create(c, &coercer) == FVM_OK);
            assert(fvm_object_define_method_rooted(c, coercer, "valueOf", scalar, 0, 0) == FVM_OK);
            fvm_value_undefined(&value);
            value.type = FVM_VALUE_OBJECT;
            value.object = coercer;
            fail_conversion = false;
            conversions = 0;
            assert(fvm_object_set(c, clip, "_width", &value) == FVM_OK && conversions == 1);
            assert(dimension(c, clip, "_width") == 60);
            fail_conversion = true;
            conversions = 0;
            assert(fvm_object_set(c, clip, "_width", &value) == FVM_ERROR && conversions == 1);
            assert(dimension(c, clip, "_width") == 60);
            /* A distinct context receives accessors and geometry, without native userdata
             * addresses. */
            FVM_Context *other = 0;
            assert(fvm_context_create(rt, &other) == FVM_OK &&
                   fvm_standard_install(other) == FVM_OK);
            unsigned char *wire = 0;
            size_t size = 0;
            assert(fvm_value_serialize(c, &cv, &wire, &size) == FVM_OK);
            assert(fvm_value_deserialize(other, wire, size, &out) == FVM_OK);
            fvm_serialized_free(wire);
            FVM_Object *copy = out.object;
            assert(fvm_gc_pin(other, copy) == FVM_OK);
            assert(dimension(other, copy, "_width") == 60);
            assign(other, copy, "_width", 25);
            assert(dimension(other, copy, "_width") == 25 && dimension(c, clip, "_width") == 60);
            assert(fvm_gc_collect(other, 0) == FVM_OK && dimension(other, copy, "_height") == 50);
            fvm_context_destroy(other);
            FVM_NativeFunction callbacks[] = {fvm_movieclip_width_get, fvm_movieclip_width_set,
                                              fvm_movieclip_height_get, fvm_movieclip_height_set};
            for (unsigned i = 0; i < 4; ++i)
            {
                unsigned long id = fvm_movieclip_callback_id(callbacks[i]);
                assert(id && fvm_movieclip_callback_from_id(id) == callbacks[i]);
            }
            FVM_Value klass, proto;
            assert(fvm_get_global(c, "MovieClip", &klass) == FVM_OK &&
                   fvm_object_get(c, klass.object, "prototype", &proto) == FVM_OK);
            assert(fvm_object_get(c, proto.object, "_width", &out) == FVM_OK &&
                   out.type == FVM_VALUE_UNDEFINED);
            FVM_Object *empty = child(c, root, "empty", 2);
            assign(c, empty, "_width", 40);
            assert(dimension(c, empty, "_width") == 0);
            assert(fvm_gc_collect(c, 0) == FVM_OK && dimension(c, clip, "_width") == 60);
            fvm_context_destroy(c);
        }
    fvm_runtime_destroy(rt);
    return 0;
}

#endif
