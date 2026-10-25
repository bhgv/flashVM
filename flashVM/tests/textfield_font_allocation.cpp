/* Face selection, future selection, assignment and replacement publish geometry atomically. */
#define main allocation_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "textfield_counted_text_allocation.cpp"

#undef main
#include "fixtures/font_bold_italic_data.h"


int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned method = 0; method < 4; ++method)
    {
        bool success = false;
        unsigned failures = 0;
        for (long boundary = 0; boundary < 4096 && !success; ++boundary)
        {
            FVM_Context *c = 0;
            if (!(fvm_context_create(rt, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *field = counted_field(c), *format = 0;
            FVM_Value v, arg, out, edit, args[3];
            if (!(fvm_standard_text_register_font_variant(c,
                                                          "Geometry",
                                                          FVM_TEXT_FONT_VARIANTS,
                                                          fvm_test_font_bold_italic,
                                                          sizeof(fvm_test_font_bold_italic),
                                                          0) == FVM_OK &&
                  fvm_construct(c, "TextFormat", 0, 0, &format) == FVM_OK && fvm_gc_pin(c, format) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&v, 40);
            if (!(fvm_object_set(c, format, "size", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_undefined(&arg);
            arg.type = FVM_VALUE_OBJECT;
            arg.object = format;
            if (!(fvm_object_call(c, field, "setNewTextFormat", 1, &arg, &out) == FVM_OK &&
                  fvm_value_string(&v, "AV") == FVM_OK && fvm_object_set(c, field, "text", &v) == FVM_OK &&
                  fvm_value_string(&v, "right") == FVM_OK && fvm_object_set(c, field, "autoSize", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_boolean(&v, 1);
            if (!(fvm_object_set(c, format, "bold", &v) == FVM_OK && fvm_object_set(c, format, "italic", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (method >= 2)
            {
                if (!(fvm_object_call(c, field, "setNewTextFormat", 1, &arg, &out) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(fvm_value_string(&edit, "AVA") == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(args, 0);
            fvm_value_number(args + 1, 2);
            if (!(fvm_value_string(args + 2, "AVA") == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            allowance = boundary;
            FVM_Result result = method == 0   ? fvm_object_call(c, field, "setTextFormat", 1, &arg, &out)
                                : method == 1 ? fvm_object_call(c, field, "setNewTextFormat", 1, &arg, &out)
                                : method == 2 ? fvm_object_set(c, field, "text", &edit)
                                              : fvm_object_call(c, field, "replaceText", 3, args, &out);
            allowance = -1;
            if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            success = result == FVM_OK;
            if (!success)
            {
                ++failures;
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            double width = success ? (method == 0 ? 64 : method == 1 ? 52 : method == 2 ? 94 : 76) : 52;
            if (!(counted_metric(c, field, "_width") == width && counted_metric(c, field, "_x") == 100 - width))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            counted_text(c, field, success && method >= 2 ? "AVA" : "AV", success && method >= 2 ? 3 : 2);
            if (!(fvm_object_call(c, field, "getTextFormat", 0, 0, &out) == FVM_OK &&
                  fvm_object_get(c, out.object, "bold", &v) == FVM_OK &&
                  v.boolean == (success && (method == 0 || method == 2))))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_call(c, field, "getNewTextFormat", 0, 0, &out) == FVM_OK &&
                  fvm_object_get(c, out.object, "italic", &v) == FVM_OK &&
                  v.boolean == (method >= 2 || (success && method == 1))))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
        }
        if (!(success && failures))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
