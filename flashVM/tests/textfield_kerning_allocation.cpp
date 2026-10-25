/* Every fallible current/future format acquisition leaves live kerning and cached geometry unchanged. */
#define main allocation_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "textfield_counted_text_allocation.cpp"

#undef main
#include <cmath>


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
    for (unsigned future = 0; future < 2; ++future)
    {
        bool success = false;
        unsigned failures = 0;
        for (long boundary = 0; boundary < 256 && !success; ++boundary)
        {
            FVM_Context *c = 0;
            if (!(fvm_context_create(rt, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *field = counted_field(c), *format = 0;
            FVM_Value value, arg, out;
            if (!(fvm_construct(c, "TextFormat", 0, 0, &format) == FVM_OK && fvm_gc_pin(c, format) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&value, 40);
            if (!(fvm_object_set(c, format, "size", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_undefined(&arg);
            arg.type = FVM_VALUE_OBJECT;
            arg.object = format;
            if (!(fvm_value_string(&value, "AV") == FVM_OK && fvm_object_set(c, field, "text", &value) == FVM_OK &&
                  fvm_object_call(c, field, "setTextFormat", 1, &arg, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            double before = counted_metric(c, field, "textWidth");
            fvm_value_boolean(&value, 1);
            if (!(fvm_object_set(c, format, "kerning", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            const char *method = future ? "setNewTextFormat" : "setTextFormat";
            allowance = boundary;
            FVM_Result result = fvm_object_call(c, field, method, 1, &arg, &out);
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
            if (!(std::fabs(counted_metric(c, field, "textWidth") - ((success && !future) ? 44.8 : before)) < 0.02))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_call(c, field, future ? "getNewTextFormat" : "getTextFormat", 0, 0, &out) == FVM_OK &&
                  fvm_object_get(c, out.object, "kerning", &value) == FVM_OK && value.type == FVM_VALUE_BOOLEAN &&
                  !!value.boolean == success))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!success)
            {
                if (!(fvm_object_call(c, field, method, 1, &arg, &out) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (future)
            {
                if (!(fvm_value_string(&value, "AVA") == FVM_OK && fvm_object_set(c, field, "text", &value) == FVM_OK &&
                      std::fabs(counted_metric(c, field, "textWidth") - 68.8) < 0.02))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
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
