/* Horizontal fitting preserves the selected parent-space anchor; paths stay local. */
#include <stdio.h>
#include <stdlib.h>
#include <cmath>

#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include "textfield_counted_text.cpp"


static void put_number(FVM_Context *c, FVM_Object *o, const char *key, double n)
{
    FVM_Value v;
    fvm_value_number(&v, n);
    if (!(fvm_object_set(c, o, key, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void put_text(FVM_Context *c, FVM_Object *o, const char *key, const char *s)
{
    FVM_Value v;
    if (!(fvm_value_string(&v, s) == FVM_OK && fvm_object_set(c, o, key, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned mode = 0; mode < 3; ++mode)
    {
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *field = counted_field(c);
        FVM_Value value, out;
        FVM_Object *format = 0;
        if (!(fvm_construct(c, "TextFormat", 0, 0, &format) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        put_number(c, format, "size", 40);
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = format;
        if (!(fvm_object_call(c, field, "setNewTextFormat", 1, &value, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        put_number(c, field, "_x", 10);
        put_number(c, field, "_y", -20);
        put_text(c, field, "text", "AV");
        const char *name = mode == 0 ? "left" : mode == 1 ? "center" : "right";
        double fraction = mode * 0.5;
        put_text(c, field, "autoSize", name);
        if (!(counted_metric(c, field, "_width") == 52 && counted_metric(c, field, "_x") == 10 + 48 * fraction))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        put_text(c, field, "text", "AVA");
        if (!(counted_metric(c, field, "_width") == 76 && counted_metric(c, field, "_x") == 10 + 24 * fraction &&
              counted_metric(c, field, "_y") == -20))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        put_number(c, field, "_x", -100);
        put_text(c, field, "text", "A");
        if (!(counted_metric(c, field, "_x") == -100 + 48 * fraction))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        put_text(c, field, "autoSize", "none");
        double x = counted_metric(c, field, "_x");
        put_text(c, field, "text", "AVAV");
        if (!(counted_metric(c, field, "_x") == x && counted_metric(c, field, "_width") == 28))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        put_text(c, field, "autoSize", name);
        put_number(c, field, "_y", 1000000);
        FVM_Value bad;
        fvm_value_number(&bad, 1000001);
        if (!(fvm_object_set(c, field, "_y", &bad) == FVM_INVALID_ARGUMENT &&
              counted_metric(c, field, "_y") == 1000000))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Path *before = 0, *after = 0;
        if (!(fvm_textfield_path(c, field, &before) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        put_number(c, field, "_x", 123);
        if (!(fvm_textfield_path(c, field, &after) == FVM_OK && fvm_path_count(before) == fvm_path_count(after)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_PathBounds b1, b2;
        if (!(fvm_path_bounds(before, &b1) == FVM_OK && fvm_path_bounds(after, &b2) == FVM_OK && b1.x_min == b2.x_min &&
              b1.x_max == b2.x_max && b1.y_min == b2.y_min && b1.y_max == b2.y_max))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_path_destroy(before);
        fvm_path_destroy(after);
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
