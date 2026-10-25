/* Extents expose signed tracking with the existing gutter and wrapping policy. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "textfield_counted_text.cpp"
#include <cmath>


int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *field = counted_field(c), *format = 0;
    FVM_Value value, args[2], out;
    (void)field;
    if (!(fvm_construct(c, "TextFormat", 0, 0, &format) == FVM_OK && fvm_gc_pin(c, format) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_string(&value, "Geometry") == FVM_OK && fvm_object_set(c, format, "font", &value) == FVM_OK))
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
    if (!(fvm_value_string(args, "AV") == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned step = 0; step < 2; ++step)
    {
        double spacing = step ? -2 : 2.5;
        fvm_value_number(&value, spacing);
        if (!(fvm_object_set(c, format, "letterSpacing", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(c, format, "getTextExtent", 1, args, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(std::fabs(counted_metric(c, out.object, "width") - (48 + 2 * spacing)) < 0.02))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(std::fabs(counted_metric(c, out.object, "textFieldWidth") - (52 + 2 * spacing)) < 0.02))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(counted_metric(c, out.object, "height") == 44 && counted_metric(c, out.object, "textFieldHeight") == 48))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_value_number(&value, 2.5);
    if (!(fvm_object_set(c, format, "letterSpacing", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args + 1, 50);
    if (!(fvm_object_call(c, format, "getTextExtent", 2, args, &out) == FVM_OK &&
          counted_metric(c, out.object, "height") == 90))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_NULL;
    if (!(fvm_object_set(c, format, "letterSpacing", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, format, "getTextExtent", 1, args, &out) == FVM_OK &&
          counted_metric(c, out.object, "width") == 48))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
