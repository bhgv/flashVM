/* Shared analytic uniform-face setup for scrolling, contours and graph ownership. */
#include <stdio.h>
#include <stdlib.h>
#ifndef FVM_TEST_TEXTFIELD_SCROLL_FIXTURE_HPP
#define FVM_TEST_TEXTFIELD_SCROLL_FIXTURE_HPP
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include "textfield_counted_text.cpp"
#include <cmath>

#ifdef FVM_HAS_FONTS


static void scroll_number(FVM_Context *c, FVM_Object *field, const char *name, double n)
{
    FVM_Value v;
    fvm_value_number(&v, n);
    if (!(fvm_object_set(c, field, name, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static FVM_Object *scroll_field(FVM_Context *c)
{
    FVM_Object *field = counted_field(c), *format = 0;
    FVM_Value v, arg, out;
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
    if (!(fvm_object_set(c, field, "multiline", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    scroll_number(c, field, "_width", 40);
    scroll_number(c, field, "_height", 94);
    if (!(fvm_value_string(&v, "AV\nAV\nAV") == FVM_OK && fvm_object_set(c, field, "text", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return field;
}


static FVM_TextFieldScrollInfo scroll_info(FVM_Context *c, FVM_Object *field)
{
    FVM_TextFieldScrollInfo i;
    i.struct_size = sizeof(i);
    if (!(fvm_textfield_scroll_info(c, field, &i) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return i;
}


static FVM_PathBounds scroll_bounds(FVM_Context *c, FVM_Object *field)
{
    FVM_Path *p = 0;
    FVM_PathBounds b;
    if (!(fvm_textfield_path(c, field, &p) == FVM_OK && fvm_path_bounds(p, &b) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_path_destroy(p);
    return b;
}
#endif
#endif
