#include "flashvm/standard_text.h"
#include "flashvm/standard.h"
#include "flashvm/avm1.h"
#include "flashvm/gc.h"
#include "fixtures/font_data.h"
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
static double get_number(FVM_Context *c, FVM_Object *o, const char *name)
{
    FVM_Value v;
    assert(fvm_object_get(c, o, name, &v) == FVM_OK && v.type == FVM_VALUE_NUMBER);
    return v.number;
}
int main(void)
{
    /* var fmt = new TextFormat("Geometry",40,0xff0000); */
    static const unsigned char code[] = {
        0x96, 5,   0,   0,   'f', 'm',  't', 0,   0x96, 5,   0,   7, 0,    0,    255, 0,
        0x96, 5,   0,   7,   40,  0,    0,   0,   0x96, 10,  0,   0, 'G',  'e',  'o', 'm',
        'e',  't', 'r', 'y', 0,   0x96, 5,   0,   7,    3,   0,   0, 0,    0x96, 12,  0,
        0,    'T', 'e', 'x', 't', 'F',  'o', 'r', 'm',  'a', 't', 0, 0x40, 0x1d, 0};
    static const unsigned char method_code[] = {
        0x96, 8,   0,    0,    'e', 'x', 't', 'e', 'n', 't', 0,   0x96, 5,    0,    7,    57, 0,
        0,    0,   0x96, 7,    0,   0,   'A', 'V', ' ', 'f', 'i', 0,    0x96, 5,    0,    7,  2,
        0,    0,   0,    0x96, 5,   0,   0,   'f', 'm', 't', 0,   0x1c, 0x96, 15,   0,    0,  'g',
        'e',  't', 'T',  'e',  'x', 't', 'E', 'x', 't', 'e', 'n', 't',  0,    0x52, 0x1d, 0};
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *c = 0, *other = 0;
    FVM_Object *fmt = 0, *empty = 0;
    FVM_Value args[2], out, v;
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &c) == FVM_OK);
    assert(fvm_context_create(runtime, &other) == FVM_OK);
    assert(fvm_standard_text_install(c) == FVM_OK);
    assert(fvm_standard_text_install(c) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_avm1_execute(c, code, sizeof(code)) == FVM_OK);
    assert(fvm_get_global(c, "fmt", &v) == FVM_OK && v.type == FVM_VALUE_OBJECT);
    fmt = v.object;
    assert(get_number(c, fmt, "size") == 40 && get_number(c, fmt, "color") == 0xff0000);
    assert(fvm_construct(c, "TextFormat", 0, 0, &empty) == FVM_OK);
    assert(fvm_gc_pin(c, empty) == FVM_OK);
    assert(fvm_object_get(c, empty, "font", &v) == FVM_OK && v.type == FVM_VALUE_NULL);
    assert(fvm_object_get(c, empty, "kerning", &v) == FVM_OK && v.type == FVM_VALUE_NULL);
    assert(fvm_value_string(&args[0], "AV fi") == FVM_OK);
#ifdef FVM_HAS_FONTS
    {
        unsigned char *copy = (unsigned char *)malloc(sizeof(fvm_test_font));
        assert(copy);
        memcpy(copy, fvm_test_font, sizeof(fvm_test_font));
        assert(fvm_standard_text_register_font(c, "Geometry", copy, sizeof(fvm_test_font), 0) ==
               FVM_OK);
        memset(copy, 0, sizeof(fvm_test_font));
        free(copy);
    }
    assert(fvm_standard_text_register_font(c, "Geometry", fvm_test_font, sizeof(fvm_test_font),
                                           0) == FVM_INVALID_ARGUMENT);
    assert(fvm_standard_text_register_font(c, "Bad", fvm_test_font, 10, 0) == FVM_INVALID_ARGUMENT);
    assert(fvm_avm1_execute(c, method_code, sizeof(method_code)) == FVM_OK);
    assert(fvm_get_global(c, "extent", &out) == FVM_OK && out.type == FVM_VALUE_OBJECT);
    assert(get_number(c, out.object, "height") == 90);
    assert(fvm_object_call(c, fmt, "getTextExtent", 1, args, &out) == FVM_OK &&
           out.type == FVM_VALUE_OBJECT);
    assert(fabs(get_number(c, out.object, "width") - 84.8) < 0.02);
    assert(get_number(c, out.object, "height") == 44);
    assert(get_number(c, out.object, "ascent") == 36 && get_number(c, out.object, "descent") == 8);
    assert(fabs(get_number(c, out.object, "textFieldWidth") - 88.8) < 0.02);
    assert(get_number(c, out.object, "textFieldHeight") == 48);
    fvm_value_number(&args[1], 57);
    assert(fvm_object_call(c, fmt, "getTextExtent", 2, args, &out) == FVM_OK);
    assert(get_number(c, out.object, "height") == 90);
    fvm_value_number(&v, -4);
    assert(fvm_object_set(c, fmt, "leading", &v) == FVM_OK);
    assert(fvm_object_call(c, fmt, "getTextExtent", 2, args, &out) == FVM_OK);
    assert(get_number(c, out.object, "height") == 84);
    fvm_value_boolean(&v, 1);
    assert(fvm_object_set(c, fmt, "bold", &v) == FVM_OK);
    assert(fvm_object_call(c, fmt, "getTextExtent", 2, args, &out) == FVM_UNSUPPORTED);
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_NULL;
    assert(fvm_object_set(c, fmt, "bold", &v) == FVM_OK);
    assert(fvm_object_call(c, fmt, "getTextExtent", 2, args, &out) == FVM_OK);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    assert(fvm_object_call(c, fmt, "getTextExtent", 2, args, &out) == FVM_OK);
#else
    (void)method_code;
    assert(fvm_standard_text_register_font(c, "Geometry", fvm_test_font, sizeof(fvm_test_font),
                                           0) == FVM_UNSUPPORTED);
    assert(fvm_object_call(c, fmt, "getTextExtent", 1, args, &out) == FVM_UNSUPPORTED);
#endif
    assert(fvm_object_call(other, fmt, "getTextExtent", 1, args, &out) == FVM_INVALID_ARGUMENT);
    assert(fvm_standard_text_install(other) == FVM_OK);
    fvm_value_number(&v, 1);
    assert(fvm_set_global(other, "TextField", &v) == FVM_OK);
    assert(fvm_standard_text_install(other) == FVM_INVALID_ARGUMENT);
    assert(fvm_gc_unpin(c, empty) == FVM_OK);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(runtime);
    return 0;
}
