/* Field gutters and per-line alignment are identical for glyphs and underline bars. */
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
    FVM_Value value, arg, out;
    if (!(fvm_construct(c, "TextFormat", 0, 0, &format) == FVM_OK))
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
    fvm_value_number(&value, 2);
    if (!(fvm_object_set(c, format, "letterSpacing", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_boolean(&value, 1);
    if (!(fvm_object_set(c, format, "underline", &value) == FVM_OK &&
          fvm_object_set(c, field, "multiline", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&arg);
    arg.type = FVM_VALUE_OBJECT;
    arg.object = format;
    if (!(fvm_value_string(&value, "AV\nA\n") == FVM_OK && fvm_object_set(c, field, "text", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned wrap = 0; wrap < 2; ++wrap)
    {
        for (unsigned align = 0; align < 3; ++align)
        {
            fvm_value_boolean(&value, wrap != 0);
            if (!(fvm_object_set(c, field, "wordWrap", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            const char *name = align == 0 ? "left" : align == 1 ? "center" : "right";
            if (!(fvm_value_string(&value, name) == FVM_OK && fvm_object_set(c, format, "align", &value) == FVM_OK &&
                  fvm_object_call(c, field, "setTextFormat", 1, &arg, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Path *path = 0;
            FVM_PathCommand point;
            if (!(fvm_textfield_path(c, field, &path) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            size_t count = fvm_path_count(path);
            if (!(count >= 10))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            double x0 = 2 + (align == 0 ? 0 : align == 1 ? 22 : 44), x1 = 2 + (align == 0 ? 0 : align == 1 ? 35 : 70);
            if (!(fvm_path_get(path, count - 10, &point) == FVM_OK && point.verb == FVM_PATH_MOVE &&
                  std::fabs(point.x[0] - x0) < 0.02 && std::fabs(point.y[0] - 41) < 0.02))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_path_get(path, count - 5, &point) == FVM_OK && point.verb == FVM_PATH_MOVE &&
                  std::fabs(point.x[0] - x1) < 0.02 && std::fabs(point.y[0] - 87) < 0.02))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_path_destroy(path);
            if (!(counted_metric(c, field, "textWidth") == 52 && counted_metric(c, field, "textHeight") == 136))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
