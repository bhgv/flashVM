/* Missing average metric rejects actual default tabs before field/cache publication. */
#include <stdio.h>
#include <stdlib.h>
#include "textfield_tabs_fixture.hpp"


int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *field = counted_field(c), *format = tabs_format(c, 0, 0);
    unsigned char bytes[sizeof(fvm_test_font)];
    std::memcpy(bytes, fvm_test_font, sizeof(bytes));
    bytes[330] = bytes[331] = 0;
    if (!(fvm_standard_text_register_font(c, "NoAverage", bytes, sizeof(bytes), 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    inset_string(c, format, "font", "NoAverage");
    inset_apply(c, field, format, "setNewTextFormat");
    inset_string(c, field, "text", "AV");
    inset_string(c, field, "autoSize", "right");
    double width = counted_metric(c, field, "textWidth"), outer = counted_metric(c, field, "_width"),
           x = counted_metric(c, field, "_x");
    FVM_Path *before = 0;
    if (!(fvm_textfield_path(c, field, &before) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t commands = fvm_path_count(before);
    fvm_path_destroy(before);
    FVM_Value text, out, args[3];
    if (!(fvm_value_string(&text, "A\tV") == FVM_OK && fvm_object_set(c, field, "text", &text) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args, 0);
    fvm_value_number(args + 1, 2);
    args[2] = text;
    if (!(fvm_object_call(c, field, "replaceText", 3, args, &out) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(counted_metric(c, field, "textWidth") == width && counted_metric(c, field, "_width") == outer &&
          counted_metric(c, field, "_x") == x))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    counted_text(c, field, "AV", 2);
    FVM_Path *after = 0;
    if (!(fvm_textfield_path(c, field, &after) == FVM_OK && fvm_path_count(after) == commands))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_path_destroy(after);
    /* Explicit tables do not require the missing metric. Clearing one does, transactionally. */
    double stops[] = {40, 80};
    format = tabs_format(c, stops, 2);
    inset_string(c, format, "font", "NoAverage");
    inset_apply(c, field, format, "setNewTextFormat");
    inset_string(c, field, "text", "A\tV");
    if (!(counted_metric(c, field, "textWidth") == 64))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *empty = tabs_array(c, 0, 0);
    FVM_Value v;
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = empty;
    if (!(fvm_object_set(c, format, "tabStops", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    v.object = format;
    if (!(fvm_object_call(c, field, "setTextFormat", 1, &v, &out) == FVM_UNSUPPORTED &&
          counted_metric(c, field, "textWidth") == 64))
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
