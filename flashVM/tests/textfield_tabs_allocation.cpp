/* Six allocation sweeps preserve copied tables, tabbed text, native caches and automatic dimensions. */
#define main allocation_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "textfield_counted_text_allocation.cpp"

#undef main
#include "flashvm/table.h"


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
            if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *field = counted_field(c), *format = 0, *array = 0;
            FVM_Value v, arg, out, args[3], edit;
            if (!(fvm_construct(c, "TextFormat", 0, 0, &format) == FVM_OK && fvm_gc_pin(c, format) == FVM_OK &&
                  fvm_construct(c, "Array", 0, 0, &array) == FVM_OK && fvm_gc_pin(c, array) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&v, 40);
            if (!(fvm_object_set(c, format, "size", &v) == FVM_OK && fvm_table_set_index(c, array, 0, &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&v, 80);
            if (!(fvm_table_set_index(c, array, 1, &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_undefined(&v);
            v.type = FVM_VALUE_OBJECT;
            v.object = array;
            if (!(fvm_object_set(c, format, "tabStops", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            arg = v;
            arg.object = format;
            if (!(fvm_object_call(c, field, "setNewTextFormat", 1, &arg, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_boolean(&v, 1);
            if (!(fvm_object_set(c, field, "multiline", &v) == FVM_OK &&
                  fvm_object_set(c, field, "wordWrap", &v) == FVM_OK && fvm_value_string(&v, "A\tV") == FVM_OK &&
                  fvm_object_set(c, field, "text", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&v, 60);
            if (!(fvm_table_set_index(c, array, 0, &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&v, 120);
            if (!(fvm_table_set_index(c, array, 1, &v) == FVM_OK))
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
                fvm_value_boolean(&v, 1);
                if (!(fvm_object_set(c, field, "autoSize", &v) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            double before_height = counted_metric(c, field, "_height"),
                   before_width = counted_metric(c, field, "textWidth");
            if (!(fvm_value_string(&edit, "A\tV\tA") == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(args, 0);
            fvm_value_number(args + 1, 3);
            args[2] = edit;
            if (method == 3)
            {
                fvm_value_boolean(&v, 1);
            }
            else if (method == 4)
            {
                fvm_value_number(&v, 74);
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
            if (!(counted_metric(c, field, "_height") ==
                  (success ? (method == 1 || method == 2 || method == 4 ? 94 : 48) : before_height)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(counted_metric(c, field, "_width") == (success ? (method == 4 ? 74 : method == 5 ? 88 : 100) : 100)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(counted_metric(c, field, "textWidth") == (success ? (method == 4 ? 60 : 84) : before_width)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            counted_text(c,
                         field,
                         success && (method == 1 || method == 2) ? "A\tV\tA" : "A\tV",
                         success && (method == 1 || method == 2) ? 5 : 3);
            if (!(fvm_object_call(c, field, "getTextFormat", 0, 0, &out) == FVM_OK &&
                  fvm_object_get(c, out.object, "tabStops", &v) == FVM_OK &&
                  fvm_table_get_index(c, v.object, 0, &out) == FVM_OK && out.number == (method || success ? 60 : 40)))
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
