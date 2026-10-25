/* Every C++ failure boundary preserves text, first-line indentation, cached metrics and field policy. */
#define main allocation_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "textfield_counted_text_allocation.cpp"

#undef main


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
    for (unsigned method = 0; method < 6; ++method)
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
            FVM_Value v, arg, out, args[3], edit;
            if (!(fvm_construct(c, "TextFormat", 0, 0, &format) == FVM_OK && fvm_gc_pin(c, format) == FVM_OK))
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
            if (!(fvm_object_call(c, field, "setNewTextFormat", 1, &arg, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_boolean(&v, 1);
            if (!(fvm_object_set(c, field, "multiline", &v) == FVM_OK &&
                  fvm_object_set(c, field, "wordWrap", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&v, 124);
            if (!(fvm_object_set(c, field, "_width", &v) == FVM_OK && fvm_value_string(&v, "A V A V") == FVM_OK &&
                  fvm_object_set(c, field, "text", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&v, 10);
            if (!(fvm_object_set(c, format, "leftMargin", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&v, 12);
            if (!(fvm_object_set(c, format, "rightMargin", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&v, 8);
            if (!(fvm_object_set(c, format, "blockIndent", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&v, 18);
            if (!(fvm_object_set(c, format, "indent", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (method)
            {
                if (!(fvm_object_call(c, field, "setTextFormat", 1, &arg, &out) == FVM_OK &&
                      fvm_object_call(c, field, "setNewTextFormat", 1, &arg, &out) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (method != 3)
            {
                if (!(fvm_value_string(&v, "left") == FVM_OK && fvm_object_set(c, field, "autoSize", &v) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            double before_height = counted_metric(c, field, "_height"),
                   before_width = counted_metric(c, field, "textWidth");
            if (!(fvm_value_string(&edit, "A V A V A") == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(args, 0);
            fvm_value_number(args + 1, 7);
            if (!(fvm_value_string(args + 2, "A V A V A") == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (method == 3)
            {
                fvm_value_boolean(&v, 1);
            }
            else if (method == 4)
            {
                fvm_value_number(&v, 104);
            }
            else
            {
                fvm_value_boolean(&v, 0);
            }
            allowance = boundary;
            FVM_Result result = method == 0   ? fvm_object_call(c, field, "setTextFormat", 1, &arg, &out)
                                : method == 1 ? fvm_object_set(c, field, "text", &edit)
                                : method == 2 ? fvm_object_call(c, field, "replaceText", 3, args, &out)
                                              : fvm_object_set(c,
                                                               field,
                                                               method == 3   ? "autoSize"
                                                               : method == 4 ? "_width"
                                                                             : "wordWrap",
                                                               &v);
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
            double height = success ? (method == 1 || method == 2 || method == 4 ? 140
                                       : method == 5                             ? 48
                                                                                 : 94)
                                    : before_height;
            if (!(counted_metric(c, field, "_height") == height &&
                  counted_metric(c, field, "_width") == (success ? (method == 4   ? 104
                                                                    : method == 5 ? 184
                                                                                  : 124)
                                                                 : 124)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(counted_metric(c, field, "textWidth") == (success ? (method == 4   ? 60
                                                                       : method == 5 ? 150
                                                                                     : 90)
                                                                    : before_width)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            counted_text(c,
                         field,
                         success && (method == 1 || method == 2) ? "A V A V A" : "A V A V",
                         success && (method == 1 || method == 2) ? 9 : 7);
            if (!(fvm_object_call(c, field, "getTextFormat", 0, 0, &out) == FVM_OK &&
                  fvm_object_get(c, out.object, "indent", &v) == FVM_OK && v.type == FVM_VALUE_NUMBER &&
                  v.number == (method || success ? 18 : 0)))
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
