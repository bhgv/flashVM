#include <stdio.h>
#include <stdlib.h>
#include "flashvm/standard_text.h"
#include "flashvm/avm1.h"
#include "flashvm/gc.h"
#include "fixtures/font_data.h"
#include <string.h>
#include <math.h>

#ifdef FVM_HAS_FONTS


static FVM_Result replace(FVM_Context *c, FVM_Object *field, double begin, double end, const char *text)
{
    FVM_Value args[3], out;
    fvm_value_number(&args[0], begin);
    fvm_value_number(&args[1], end);
    if (!(fvm_value_string(&args[2], text) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return fvm_object_call(c, field, "replaceText", 3, args, &out);
}


static void set_text(FVM_Context *c, FVM_Object *field, const char *text)
{
    FVM_Value v;
    if (!(fvm_value_string(&v, text) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set(c, field, "text", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void expect_text(FVM_Context *c, FVM_Object *field, const char *text)
{
    FVM_Value v;
    if (!(fvm_object_get(c, field, "text", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(v.type == FVM_VALUE_STRING && strcmp(v.string, text) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static double metric(FVM_Context *c, FVM_Object *field, const char *name)
{
    FVM_Value v;
    if (!(fvm_object_get(c, field, name, &v) == FVM_OK && v.type == FVM_VALUE_NUMBER))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return v.number;
}
#endif


int main(void)
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *c = 0;
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
#ifdef FVM_HAS_FONTS
    { /* field.replaceText(0, 1, "B"), with arguments popped in AS order. */
        static const unsigned char code[] = {
            0x96, 3,    0,    0, 'B', 0,   0x96, 5,   0,   7,   1,    0,   0,   0,   0x96, 5,   0,   7,    0,    0,
            0,    0,    0x96, 5, 0,   7,   3,    0,   0,   0,   0x96, 7,   0,   0,   'f',  'i', 'e', 'l',  'd',  0,
            0x1c, 0x96, 13,   0, 0,   'r', 'e',  'p', 'l', 'a', 'c',  'e', 'T', 'e', 'x',  't', 0,   0x52, 0x17, 0};
        FVM_Object *field = 0, *format = 0;
        FVM_Value v, old, out, args[3];
        FVM_Path *path = 0;
        FVM_PathBounds bounds;
        unsigned int begin, end;
        char expected[32], limit[4098], large[16389];
        double before;
        if (!(fvm_standard_text_register_font(c, "Geometry", fvm_test_font, sizeof(fvm_test_font), 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_textfield_create(c, 0, 60, 100, &field) == FVM_OK))
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
        set_text(c, field, "AV fi");
        if (!(fvm_object_get(c, field, "text", &old) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        before = metric(c, field, "textWidth");
        if (!(fvm_avm1_execute(c, code, sizeof(code)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        expect_text(c, field, "BV fi");
        /* A and B have equal advances; the AS2 disabled-kerning default removes the old AV adjustment. */
        if (!(metric(c, field, "textWidth") == before))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(strcmp(old.string, "AV fi") == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(replace(c, field, 2, 5, "") == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        expect_text(c, field, "BV");
        if (!(replace(c, field, 2, 2, " fi") == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        expect_text(c, field, "BV fi");
        /* Every valid ASCII range, including empty insertion and end boundary. */
        for (begin = 0; begin <= 5; ++begin)
        {
            for (end = begin; end <= 5; ++end)
            {
                set_text(c, field, "ABCDE");
                memcpy(expected, "ABCDE", begin);
                memcpy(expected + begin, "fi", 2);
                strcpy(expected + begin + 2, "ABCDE" + end);
                if (!(replace(c, field, begin, end, "fi") == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                expect_text(c, field, expected);
            }
        }
        set_text(c, field, "A\xf0\x9f\x98\x80\xd0\x91");
        if (!(metric(c, field, "length") == 4))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        before = metric(c, field, "textWidth");
        if (!(replace(c, field, 1, 2, "B") == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(replace(c, field, 2, 3, "B") == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(replace(c, field, -1, 1, "B") == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(replace(c, field, 0.5, 1, "B") == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(replace(c, field, 3, 1, "B") == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(replace(c, field, 0, 5, "B") == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(replace(c, field, 0, HUGE_VAL, "B") == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(replace(c, field, 0, 1, "\xc0\xaf") == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(metric(c, field, "textWidth") == before))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(replace(c, field, 1, 3, "fi") == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        expect_text(c, field, "Afi\xd0\x91");
        if (!(replace(c, field, 3, 4, "\xf0\x9f\x98\x80") == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(metric(c, field, "length") == 5))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        /* Partial format reads are independent uniform snapshots. */
        fvm_value_number(&args[0], 1);
        fvm_value_number(&args[1], 3);
        if (!(fvm_object_call(c, field, "getTextFormat", 2, args, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(metric(c, out.object, "size") == 12))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(c, field, "getTextFormat", 1, args, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&args[0], 4);
        if (!(fvm_object_call(c, field, "getTextFormat", 1, args, &out) == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&args[0], 5);
        if (!(fvm_object_call(c, field, "getTextFormat", 1, args, &out) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&args[0], 1);
        fvm_value_number(&args[1], 1);
        if (!(fvm_object_call(c, field, "getTextFormat", 2, args, &out) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        /* Aliasing the old text never reads freed memory or loses the suffix. */
        if (!(fvm_object_get(c, field, "text", &old) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(replace(c, field, 0, 5, old.string) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(strcmp(old.string, "Afi\xf0\x9f\x98\x80") == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(replace(c, field, 0, 5, "") == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(metric(c, field, "length") == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(replace(c, field, 0, 0, "") == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        /* Inserting in an empty field uses future style; editing nonempty text
         * preserves current style even when the future style differs. */
        if (!(fvm_construct(c, "TextFormat", 0, 0, &format) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(c, format) == FVM_OK))
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
        fvm_value_undefined(&args[0]);
        args[0].type = FVM_VALUE_OBJECT;
        args[0].object = format;
        if (!(fvm_object_call(c, field, "setNewTextFormat", 1, args, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(replace(c, field, 0, 0, "fi") == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(metric(c, field, "textWidth") == 28))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&v, 20);
        if (!(fvm_object_set(c, format, "size", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(c, field, "setNewTextFormat", 1, args, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(replace(c, field, 0, 2, "B") == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(metric(c, field, "textWidth") == 24))
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
        fvm_value_number(&v, 61);
        if (!(fvm_object_set(c, field, "_width", &v) == FVM_OK))
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
        if (!(replace(c, field, 0, 1, "AV fi") == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(metric(c, field, "textHeight") == 90))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(replace(c, field, 0, 5, "fi") == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(metric(c, field, "textHeight") == 44))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_unpin(c, format) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        /* Resource ceilings fail atomically. */
        memset(limit, 'A', 4096);
        limit[4096] = 0;
        set_text(c, field, limit);
        if (!(replace(c, field, 4096, 4096, "B") == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(metric(c, field, "length") == 4096))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(replace(c, field, 0, 4096, "B") == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(replace(c, field, 0, 1, "fi") == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_path_bounds(path, &bounds) == FVM_OK && bounds.x_max == 22))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_path_destroy(path);
        /* Byte and UTF-16 ceilings differ: 4096 supplementary scalars occupy
         * 16384 UTF-8 bytes / 8192 UTF-16 units. No narrowing or partial pair. */
        for (begin = 0; begin < 4096; ++begin)
        {
            memcpy(large + begin * 4, "\xf0\x9f\x98\x80", 4);
        }
        large[16384] = 0;
        set_text(c, field, large);
        if (!(metric(c, field, "length") == 8192))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(replace(c, field, 8192, 8192, "B") == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(replace(c, field, 8191, 8192, "B") == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(replace(c, field, 0, 8192, "fi") == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        memset(large, 'A', 16385);
        large[16385] = 0;
        if (!(replace(c, field, 0, 2, large) == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        expect_text(c, field, "fi");
        if (!(fvm_object_get(c, field, "text", &old) == FVM_OK))
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
        if (!(strcmp(old.string, "fi") == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
#endif
    fvm_context_destroy(c);
    fvm_runtime_destroy(runtime);
    return 0;
}
