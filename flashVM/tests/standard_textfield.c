#include <stdio.h>
#include <stdlib.h>
#include "flashvm/standard_text.h"
#include "flashvm/gc.h"
#include "flashvm/avm1.h"
#include "fixtures/font_data.h"
#include <string.h>
#include <math.h>


static double get_number(FVM_Context *c, FVM_Object *o, const char *name)
{
    FVM_Value v;
    if (!(fvm_object_get(c, o, name, &v) == FVM_OK && v.type == FVM_VALUE_NUMBER))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return v.number;
}


int main(void)
{ /* field.text = "AV fi"; measured = field.textWidth; dynamic AVM1 accessors. */
    static const unsigned char code[] = {
        0x96, 7,    0,    0,   'f', 'i', 'e', 'l', 'd',  0,   0x1c, 0x96, 6,    0,    0,   't',  'e',  'x',
        't',  0,    0x96, 7,   0,   0,   'A', 'V', ' ',  'f', 'i',  0,    0x4f, 0x96, 10,  0,    0,    'm',
        'e',  'a',  's',  'u', 'r', 'e', 'd', 0,   0x96, 7,   0,    0,    'f',  'i',  'e', 'l',  'd',  0,
        0x1c, 0x96, 11,   0,   0,   't', 'e', 'x', 't',  'W', 'i',  'd',  't',  'h',  0,   0x4e, 0x1d, 0};
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *c = 0;
    FVM_Object *field = 0;
    FVM_Value v;
    FVM_Path *path = 0;
    if (!(fvm_config_init(&config) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&config, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(runtime, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_text_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#ifndef FVM_HAS_FONTS
    (void)code;
    if (!(fvm_textfield_create(c, 0, 100, 100, &field) == FVM_UNSUPPORTED && !field))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#else
    {
        FVM_Object *fmt = 0;
        FVM_Value args[3], out, retained;
        FVM_PathBounds bounds;
        FVM_PathCommand command;
        unsigned long count_before, count_after;
        if (!(fvm_standard_text_register_font(c, "Geometry", fvm_test_font, sizeof(fvm_test_font), 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_textfield_create(c, "Unknown", 100, 100, &field) == FVM_NOT_FOUND && !field))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_textfield_create(c, 0, 61, 200, &field) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&v);
        v.type = FVM_VALUE_OBJECT;
        v.object = field;
        if (!(fvm_set_global(c, "field", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_construct(c, "TextFormat", 0, 0, &fmt) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(c, fmt) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        /* Preserve this fixture's explicitly kerned geometry while testing the other field APIs. */
        fvm_value_boolean(&v, 1);
        if (!(fvm_object_set(c, fmt, "kerning", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&v, 40);
        if (!(fvm_object_set(c, fmt, "size", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&args[0]);
        args[0].type = FVM_VALUE_OBJECT;
        args[0].object = fmt;
        if (!(fvm_object_call(c, field, "setNewTextFormat", 1, args, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_avm1_execute(c, code, sizeof(code)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_get_global(c, "measured", &v) == FVM_OK && fabs(v.number - 84.8) < 0.02))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(get_number(c, field, "textHeight") == 44 && get_number(c, field, "length") == 5))
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
        if (!(fvm_object_set(c, field, "wordWrap", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(get_number(c, field, "textHeight") == 90))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fabs(get_number(c, field, "textWidth") - 56.8) < 0.02))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, field, "text", &retained) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_value_string(&v, "fi") == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set(c, field, "text", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(strcmp(retained.string, "AV fi") == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(get_number(c, field, "textWidth") == 28))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_boolean(&v, 1);
        if (!(fvm_object_set(c, field, "html", &v) == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, field, "autoSize", &out) == FVM_OK && out.type == FVM_VALUE_STRING &&
              strcmp(out.string, "none") == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(c, field, "replaceText", 0, 0, &out) == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&v, 999);
        if (!(fvm_object_set(c, field, "textWidth", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(get_number(c, field, "textWidth") == 28))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        /* Current and future formats are distinct snapshots, not caller objects. */
        fvm_value_number(&v, 20);
        if (!(fvm_object_set(c, fmt, "size", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(c, field, "setTextFormat", 1, args, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(get_number(c, field, "textWidth") == 14))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(c, field, "getNewTextFormat", 0, 0, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(get_number(c, out.object, "size") == 40))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(c, field, "getTextFormat", 0, 0, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(get_number(c, out.object, "size") == 20))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_boolean(&v, 1);
        if (!(fvm_object_set(c, fmt, "bold", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(c, field, "setTextFormat", 1, args, &out) == FVM_NOT_FOUND))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(get_number(c, field, "textWidth") == 14))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&v);
        v.type = FVM_VALUE_NULL;
        if (!(fvm_object_set(c, fmt, "bold", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_value_string(&v, "A\xf0\x9f\x98\x80") == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set(c, field, "text", &v) == FVM_OK && get_number(c, field, "length") == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_value_string(&v, "\xc0\xaf") == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set(c, field, "text", &v) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(get_number(c, field, "length") == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&v, 0xff0000);
        if (!(fvm_object_set(c, field, "textColor", &v) == FVM_OK && get_number(c, field, "textColor") == 0xff0000))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_value_string(&v, "B") == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set(c, field, "text", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_textfield_path(c, field, &path) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_path_bounds(path, &bounds) == FVM_OK && bounds.x_max == 22))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_path_get(path, 1, &command) == FVM_OK && command.verb == FVM_PATH_QUADRATIC))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_unpin(c, fmt) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(get_number(c, field, "textWidth") == 24))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_object_count(c, &count_before) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&v);
        if (!(fvm_set_global(c, "field", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_object_count(c, &count_after) == FVM_OK && count_after < count_before))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        /* Export survives both resource collection and context destruction. */
    }
#endif
    fvm_context_destroy(c);
    fvm_runtime_destroy(runtime);
#ifdef FVM_HAS_FONTS
    {
        FVM_PathBounds bounds;
        if (!(fvm_path_bounds(path, &bounds) == FVM_OK && bounds.x_max == 22))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
#endif
    fvm_path_destroy(path);
    return 0;
}
