/* Seventy-two real AVM1 linearRGB gradient calls compare interpreter/manual/native JIT with native-entry evidence. */
#define FVM_CLASS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_classes.cpp"
#include "gradient_linear_fixture.hpp"


int main()
{
    if (!fvm_jit_available())
    {
        return 0;
    }
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned loaded = 0; loaded < 2; ++loaded)
    {
        for (unsigned stroke = 0; stroke < 2; ++stroke)
        {
            for (unsigned mode = 0; mode < 3; ++mode)
            {
                for (unsigned radial = 0; radial < 2; ++radial)
                {
                    for (class_mode = 0; class_mode < 3; ++class_mode)
                    {
                        FVM_Context *c = 0;
                        if (!(fvm_context_create(rt, &c) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (loaded)
                        {
                            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (!(fvm_standard_install(c) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        FVM_Object *clip = 0;
                        if (!(fvm_movieclip_create(c, "root", 1, &clip) == FVM_OK && fvm_gc_pin(c, clip) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        FVM_Value args[8], out;
                        linear_arguments(c, args, mode, radial != 0);
                        if (stroke)
                        {
                            FVM_Value width;
                            fvm_value_number(&width, 4);
                            if (!(fvm_object_call(c, clip, "lineStyle", 1, &width, &out) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        ref(&out, clip);
                        if (!(fvm_set_global(c, "paintClip", &out) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        const char *names[] = {"paintColors", "paintAlphas", "paintRatios", "paintMatrix"};
                        for (unsigned i = 0; i < 4; ++i)
                        {
                            if (!(fvm_set_global(c, names[i], args + i + 1) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        Program code;
                        code.number(0);
                        code.text("linearRGB");
                        code.text(mode == 1 ? "reflect" : mode == 2 ? "repeat" : "pad");
                        for (unsigned i = 4; i; --i)
                        {
                            code.variable(names[i - 1]);
                        }
                        code.text(radial ? "radial" : "linear");
                        code.number(8);
                        code.variable("paintClip");
                        code.text(stroke ? "lineGradientStyle" : "beginGradientFill");
                        code.op(0x52);
                        code.op(0x17);
                        code.number(42);
                        code.finish();
                        out = class_run(c, code);
                        if (!(out.type == FVM_VALUE_NUMBER && out.number == 42))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        FVM_Object *record =
                            clip->resource_result.object->properties[stroke ? "lineGradient" : "gradient"].object;
                        if (!(record && record->properties["interpolation"].number == 1))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (mode)
                        {
                            if (!(record->properties["spread"].number == mode))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                        {
                            if (!(record->properties.find("spread") == record->properties.end()))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (stroke)
                        {
                            line_gradient_geometry(c, clip);
                            linear_pixels(c, clip, mode, radial != 0, true);
                        }
                        else
                        {
                            geometry(c, clip);
                            linear_pixels(c, clip, mode, radial != 0, false);
                        }
                        if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth &&
                              !c->avm1_execution_depth))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        fvm_context_destroy(c);
                    }
                }
            }
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
