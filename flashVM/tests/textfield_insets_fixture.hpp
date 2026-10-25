/* Shared owned-field fixture for paragraph inset contracts. */
#define FVM_FONT_FACES_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "textfield_font_faces.cpp"


static void inset_number(FVM_Context *c, FVM_Object *o, const char *name, double n)
{
    FVM_Value v;
    fvm_value_number(&v, n);
    if (!(fvm_object_set(c, o, name, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void inset_string(FVM_Context *c, FVM_Object *o, const char *name, const char *s)
{
    FVM_Value v;
    if (!(fvm_value_string(&v, s) == FVM_OK && fvm_object_set(c, o, name, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void inset_bool(FVM_Context *c, FVM_Object *o, const char *name, bool b)
{
    FVM_Value v;
    fvm_value_boolean(&v, b);
    if (!(fvm_object_set(c, o, name, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void inset_apply(FVM_Context *c, FVM_Object *field, FVM_Object *format, const char *method)
{
    FVM_Value v, out;
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = format;
    if (!(fvm_object_call(c, field, method, 1, &v, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static FVM_Object *inset_format(FVM_Context *c)
{
    FVM_Object *o = 0;
    if (!(fvm_construct(c, "TextFormat", 0, 0, &o) == FVM_OK && fvm_gc_pin(c, o) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    inset_number(c, o, "size", 40);
    inset_number(c, o, "leftMargin", 10);
    inset_number(c, o, "rightMargin", 12);
    inset_number(c, o, "blockIndent", 8);
    return o;
}
#ifndef FVM_INSETS_FIXTURE_ONLY


int main()
{
    return 0;
}
#endif
