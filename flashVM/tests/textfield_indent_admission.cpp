/* Positive/negative SWF gates, scalar bounds, rollback and packed-state consistency. */
#define FVM_INSETS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "textfield_insets_fixture.hpp"
#include "../src/standard/text_state.hpp"
#include "../src/standard/text_constants.h"
#include <limits>


int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 6; version <= 8; ++version)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char swf[] = {'F', 'W', 'S', 0, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *field = counted_field(c), *format = inset_format(c);
        inset_number(c, format, "indent", 18);
        inset_apply(c, field, format, "setNewTextFormat");
        inset_string(c, field, "text", "AV");
        inset_number(c, format, "indent", -18);
        FVM_Value arg, out, v;
        fvm_value_undefined(&arg);
        arg.type = FVM_VALUE_OBJECT;
        arg.object = format;
        if (!(fvm_object_call(c, field, "setTextFormat", 1, &arg, &out) == (version == 8 ? FVM_OK : FVM_UNSUPPORTED)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(c, field, "getTextFormat", 0, 0, &out) == FVM_OK &&
              counted_metric(c, out.object, "indent") == (version == 8 ? -18 : 18)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_TextFieldState restore;
        if (!(fvm_textfield_snapshot(field, restore) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        restore.current.indent = -18;
        restore.current.alignment |= FVM_TEXT_STYLE_HAS_INDENT;
        FVM_Context *d = 0;
        FVM_Object *o = 0;
        if (!(fvm_context_create(rt, &d) == FVM_OK && fvm_load_memory(d, swf, sizeof(swf)) == FVM_OK &&
              fvm_standard_text_install(d) == FVM_OK && fvm_object_create(d, &o) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        o->resource_kind = FVM_RESOURCE_TEXT_FIELD;
        if (!(fvm_textfield_restore(d, o, restore) == (version == 8 ? FVM_OK : FVM_UNSUPPORTED)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version != 8)
        {
            if (!(!o->resource && d->fonts.empty()))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_context_destroy(d);
        if (version == 8)
        {
            if (!(counted_metric(c, field, "textWidth") == 30))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            swf[3] = 7;
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK &&
                  fvm_object_get(c, field, "textWidth", &v) == FVM_UNSUPPORTED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Path *path = 0;
            if (!(fvm_textfield_path(c, field, &path) == FVM_UNSUPPORTED && !path))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            inset_number(c, format, "indent", 18);
            inset_apply(c, field, format, "setTextFormat");
            if (!(counted_metric(c, field, "textWidth") == 66))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_context_destroy(c);
    }
    FVM_Context *c = 0;
    if (!(fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *field = counted_field(c), *format = inset_format(c);
    inset_number(c, format, "indent", 18);
    inset_apply(c, field, format, "setNewTextFormat");
    inset_string(c, field, "text", "AV");
    FVM_TextFieldState good;
    if (!(fvm_textfield_snapshot(field, good) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned kind = 0; kind < 8; ++kind)
    {
        FVM_TextFieldState state = good;
        FVM_Context *d = 0;
        FVM_Object *o = 0;
        if (!(fvm_context_create(rt, &d) == FVM_OK && fvm_standard_text_install(d) == FVM_OK &&
              fvm_object_create(d, &o) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (kind == 0)
        {
            state.current.indent = 4097;
        }
        if (kind == 1)
        {
            state.current.indent = 0.5;
        }
        if (kind == 2)
        {
            state.current.indent = std::numeric_limits<double>::quiet_NaN();
        }
        if (kind == 3)
        {
            state.current.indent = std::numeric_limits<double>::infinity();
        }
        if (kind == 4)
        {
            state.current.indent = 0;
        }
        if (kind == 5)
        {
            state.current.alignment &= ~FVM_TEXT_STYLE_HAS_INDENT;
        }
        if (kind == 6)
        {
            state.current.indent = -19;
        }
        if (kind == 7)
        {
            state.current.alignment |= 2048UL;
        }
        if (!(fvm_textfield_restore(d, o, state) == (kind == 6 ? FVM_UNSUPPORTED : FVM_INVALID_ARGUMENT) &&
              !o->resource && d->fonts.empty()))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(d);
    }
    inset_number(c, format, "indent", 18);
    inset_apply(c, field, format, "setTextFormat");
    FVM_Value v;
    if (!(fvm_value_string(&v, "\xd7\x90") == FVM_OK && fvm_object_set(c, field, "text", &v) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    counted_text(c, field, "AV", 2);
    const double bad[] = {-4097, 4097, 0.5};
    for (unsigned i = 0; i < 3; ++i)
    {
        inset_number(c, format, "indent", bad[i]);
        FVM_Value arg, out;
        fvm_value_undefined(&arg);
        arg.type = FVM_VALUE_OBJECT;
        arg.object = format;
        if (!(fvm_object_call(c, field, "setTextFormat", 1, &arg, &out) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        counted_text(c, field, "AV", 2);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
