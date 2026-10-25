/* Shared counted edit and format calls for transactional text-range observer tests. */
#include <stdio.h>
#include <stdlib.h>
#ifndef FVM_TEST_TEXTFIELD_EDIT_EVENTS_FIXTURE_HPP
#define FVM_TEST_TEXTFIELD_EDIT_EVENTS_FIXTURE_HPP
#include "textfield_geometry_events_fixture.hpp"

#ifdef FVM_HAS_FONTS


static FVM_Result edit_text(FVM_Context *c, FVM_Object *field, const char *text, size_t size)
{
    FVM_Value value;
    if (!(fvm_value_string_bytes(&value, text, size) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return fvm_object_set(c, field, "text", &value);
}


static FVM_Result
edit_replace(FVM_Context *c, FVM_Object *field, double begin, double end, const char *text, size_t size)
{
    FVM_Value args[3], out;
    fvm_value_number(args, begin);
    fvm_value_number(args + 1, end);
    if (!(fvm_value_string_bytes(args + 2, text, size) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return fvm_object_call(c, field, "replaceText", 3, args, &out);
}


static FVM_Object *edit_format(FVM_Context *c, double size)
{
    FVM_Object *format = 0;
    if (!(fvm_construct(c, "TextFormat", 0, 0, &format) == FVM_OK && fvm_gc_pin(c, format) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    scroll_number(c, format, "size", size);
    return format;
}


static FVM_Result
edit_apply(FVM_Context *c, FVM_Object *field, FVM_Object *format, const char *method = "setTextFormat")
{
    FVM_Value arg, out;
    fvm_value_undefined(&arg);
    arg.type = FVM_VALUE_OBJECT;
    arg.object = format;
    return fvm_object_call(c, field, method, 1, &arg, &out);
}
#endif
#endif
