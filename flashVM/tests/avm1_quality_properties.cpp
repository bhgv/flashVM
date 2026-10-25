/* Classic display-property indices test target resolution, coercion ordering, scalar rules and GC. */
#define FVM_AVM1_TARGET_FIXTURE_ONLY
#include <stdlib.h>
#include "avm1_target_scope.cpp"


static void inputs(FVM_Context *c, const FVM_Value &target, const FVM_Value &index, const FVM_Value *value)
{
    if (!(fvm_set_global(c, "p_target", &target) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_set_global(c, "p_index", &index) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (value)
    {
        if (!(fvm_set_global(c, "p_value", value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
}


static FVM_Value get(FVM_Context *c, FVM_Object *root, const FVM_Value &path, const FVM_Value &index)
{
    inputs(c, path, index, 0);
    Bytes b;
    text(b, "result");
    global(b, "p_target");
    global(b, "p_index");
    b.push_back(0x22);
    b.push_back(0x1d);
    if (!(execute(c, root, b) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value out;
    if (!(fvm_object_get(c, root, "result", &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return out;
}


static FVM_Result
set(FVM_Context *c, FVM_Object *root, const FVM_Value &path, const FVM_Value &index, const FVM_Value &value)
{
    inputs(c, path, index, &value);
    Bytes b;
    global(b, "p_target");
    global(b, "p_index");
    global(b, "p_value");
    b.push_back(0x23);
    return execute(c, root, b);
}

#include <stdio.h>
#include "flashvm/serialization.h"
#include "flashvm/stage.h"
#include "flashvm/execution_mode.h"
#include "../src/standard/movieclip_quality.hpp"


static void expect_quality(FVM_Context *c, FVM_StageQuality wanted)
{
    FVM_StageQuality actual;
    static unsigned n = 0;
    ++n;
    if (!(fvm_stage_get_quality(c, &actual) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (actual != wanted)
    {
        fprintf(stderr, "quality check %u got %d wanted %d\n", n, (int)actual, (int)wanted);
    }
    if (!(actual == wanted))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned mode = 0; mode < 2; ++mode)
        {
            FVM_Context *c = 0;
            if (!(fvm_context_create(rt, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            swf[3] = (unsigned char)version;
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_standard_install(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_execution_set_mode(c, mode ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *root = 0;
            if (!(fvm_movieclip_create(c, "root", 3, &root) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value rv;
            fvm_value_undefined(&rv);
            rv.type = FVM_VALUE_OBJECT;
            rv.object = root;
            if (!(fvm_set_global(c, "root", &rv) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *clip = child(c, root, "child", 1);
            FVM_Value path, index, value, out;
            fvm_value_string(&path, "child");
            const char *labels[] = {"low", "MeDiUm", "HIGH", "bEsT"};
            FVM_StageQuality qualities[] = {
                FVM_STAGE_QUALITY_LOW, FVM_STAGE_QUALITY_MEDIUM, FVM_STAGE_QUALITY_HIGH, FVM_STAGE_QUALITY_BEST};
            for (unsigned i = 0; i < 4; ++i)
            {
                fvm_value_string(&value, labels[i]);
                if (!(fvm_object_set(c, clip, "_quality", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                expect_quality(c, qualities[i]);
                if (!(fvm_object_get(c, root, "_highquality", &out) == FVM_OK && out.number == (i == 3   ? 2
                                                                                                : i == 2 ? 1
                                                                                                         : 0)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_value_number(&index, 19);
                out = get(c, root, path, index);
                if (!(out.type == FVM_VALUE_STRING && std::strlen(out.string) == std::strlen(labels[i])))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            fvm_value_string_bytes(&value, "low\0suffix", 10);
            if (!(fvm_object_set(c, clip, "_quality", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            expect_quality(c, FVM_STAGE_QUALITY_BEST);
            fvm_value_string(&value, " low");
            if (!(fvm_object_set(c, clip, "_quality", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            expect_quality(c, FVM_STAGE_QUALITY_BEST);
            double numbers[] = {0,
                                -0.0,
                                -2,
                                1.5,
                                1.5001,
                                std::numeric_limits<double>::infinity(),
                                -std::numeric_limits<double>::infinity()};
            FVM_StageQuality expected[] = {FVM_STAGE_QUALITY_LOW,
                                           FVM_STAGE_QUALITY_LOW,
                                           FVM_STAGE_QUALITY_HIGH,
                                           FVM_STAGE_QUALITY_HIGH,
                                           FVM_STAGE_QUALITY_BEST,
                                           FVM_STAGE_QUALITY_BEST,
                                           FVM_STAGE_QUALITY_HIGH};
            for (unsigned i = 0; i < 7; ++i)
            {
                fvm_value_number(&value, numbers[i]);
                if (!(fvm_object_set(c, clip, "_highquality", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                expect_quality(c, expected[i]);
                fvm_value_number(&index, 16);
                if (!(set(c, root, path, index, value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                expect_quality(c, expected[i]);
            }
            fvm_value_number(&value, std::numeric_limits<double>::quiet_NaN());
            if (!(set(c, root, path, index, value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            expect_quality(c, FVM_STAGE_QUALITY_HIGH);
            fvm_value_undefined(&value);
            value.type = FVM_VALUE_NULL;
            if (!(fvm_object_set(c, clip, "_highquality", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            expect_quality(c, FVM_STAGE_QUALITY_HIGH);
            if (!(set(c, root, path, index, value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            expect_quality(c, version < 7 ? FVM_STAGE_QUALITY_LOW : FVM_STAGE_QUALITY_HIGH);
            fvm_value_string(&value, "medium");
            fvm_value_number(&index, 19);
            if (!(set(c, root, path, index, value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            expect_quality(c, FVM_STAGE_QUALITY_MEDIUM);
            /* Named native coercion rejects explicit collection; action coercion allows a rooted safe point. */
            FVM_Object *coercer = 0;
            if (!(fvm_construct(c, "Object", 0, 0, &coercer) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_extensions_install(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            for (unsigned label = 0; label < 2; ++label)
            {
                Bytes body;
                integer(body, 0);
                global(body, "gc");
                text(body, "collect");
                body.push_back(0x52);
                body.push_back(0x17);
                if (label)
                {
                    text(body, "low");
                }
                else
                {
                    integer(body, 2);
                }
                body.push_back(0x3e);
                body.push_back(0);
                FVM_Object *fn = 0;
                if (!(fvm_function_create_avm1(c, body.begin(), body.size(), 0, 0, 0, &fn) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_value_undefined(&value);
                value.type = FVM_VALUE_OBJECT;
                value.object = fn;
                if (!(fvm_object_set(c, coercer, label ? "toString" : "valueOf", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            fvm_value_undefined(&value);
            value.type = FVM_VALUE_OBJECT;
            value.object = coercer;
            if (!(fvm_set_global(c, "coercer", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_set(c, clip, "_highquality", &value) == FVM_INVALID_ARGUMENT))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            expect_quality(c, FVM_STAGE_QUALITY_MEDIUM);
            if (!(fvm_object_set(c, clip, "_quality", &value) == FVM_INVALID_ARGUMENT))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            expect_quality(c, FVM_STAGE_QUALITY_MEDIUM);
            fvm_value_number(&index, 16);
            if (!(set(c, root, path, index, value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            expect_quality(c, FVM_STAGE_QUALITY_BEST);
            fvm_value_number(&index, 19);
            if (!(set(c, root, path, index, value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            expect_quality(c, FVM_STAGE_QUALITY_LOW);
            /* Serialized accessors retain their identity, not the source context stage state. */
            FVM_Context *other = 0;
            if (!(fvm_context_create(rt, &other) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_standard_install(other) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            unsigned char *wire = 0;
            size_t wire_size = 0;
            if (!(fvm_value_serialize(c, &rv, &wire, &wire_size) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_value_deserialize(other, wire, wire_size, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_serialized_free(wire);
            FVM_Object *copy = out.object;
            if (!(fvm_gc_pin(other, copy) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            expect_quality(other, FVM_STAGE_QUALITY_HIGH);
            if (!(fvm_object_get(other, copy, "_quality", &out) == FVM_OK && !std::strcmp(out.string, "HIGH")))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string(&value, "best");
            if (!(fvm_object_set(other, copy, "_quality", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            expect_quality(other, FVM_STAGE_QUALITY_BEST);
            expect_quality(c, FVM_STAGE_QUALITY_LOW);
            fvm_value_number(&value, 0);
            if (!(fvm_object_set(other, copy, "_highquality", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            expect_quality(other, FVM_STAGE_QUALITY_LOW);
            fvm_context_destroy(other);
            /* Intrinsic actions bypass an own named property shadow. */
            clip->properties["_quality"] = value;
            fvm_value_string(&value, "best");
            if (!(set(c, root, path, index, value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            expect_quality(c, FVM_STAGE_QUALITY_BEST);
            /* Stable graph callback IDs must round trip without colliding with methods. */
            FVM_NativeFunction callbacks[] = {fvm_movieclip_quality_get,
                                              fvm_movieclip_quality_set,
                                              fvm_movieclip_highquality_get,
                                              fvm_movieclip_highquality_set};
            for (unsigned i = 0; i < 4; ++i)
            {
                unsigned long id = fvm_movieclip_callback_id(callbacks[i]);
                if (!(id && fvm_movieclip_callback_from_id(id) == callbacks[i]))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
