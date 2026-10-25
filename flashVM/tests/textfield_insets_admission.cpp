/* Reject inconsistent optional inset state before fonts or native attachment; accept legacy zeros. */
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
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *field = counted_field(c), *format = inset_format(c);
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
        FVM_Context *d = 0;
        FVM_Object *o = 0;
        if (!(fvm_context_create(rt, &d) == FVM_OK && fvm_standard_text_install(d) == FVM_OK &&
              fvm_object_create(d, &o) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_TextFieldState state = good;
        if (kind == 0)
        {
            state.current.left_margin = -1;
        }
        if (kind == 1)
        {
            state.current.right_margin = 4097;
        }
        if (kind == 2)
        {
            state.current.block_indent = 0.5;
        }
        if (kind == 3)
        {
            state.current.left_margin = std::numeric_limits<double>::quiet_NaN();
        }
        if (kind == 4)
        {
            state.current.right_margin = std::numeric_limits<double>::infinity();
        }
        if (kind == 5)
        {
            state.current.alignment &= ~FVM_TEXT_STYLE_HAS_INSETS;
        }
        if (kind == 6)
        {
            state.current.left_margin = state.current.right_margin = state.current.block_indent = 0;
        }
        if (kind == 7)
        {
            state.current.alignment |= 1024UL;
        }
        if (!(fvm_textfield_restore(d, o, state) == FVM_INVALID_ARGUMENT && !o->resource && d->fonts.empty()))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(d);
    }
    FVM_Context *d = 0;
    FVM_Object *o = 0;
    if (!(fvm_context_create(rt, &d) == FVM_OK && fvm_standard_text_install(d) == FVM_OK &&
          fvm_object_create(d, &o) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    good.current.left_margin = good.current.right_margin = good.current.block_indent = 0;
    good.current.alignment &= ~FVM_TEXT_STYLE_HAS_INSETS;
    good.future = good.current;
    if (!(fvm_textfield_restore(d, o, good) == FVM_OK && o->resource))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(d);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
