/* Native TextFormat.indent shifts only first paragraph lines and participates in fitting. */
#define FVM_INSETS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "textfield_insets_fixture.hpp"


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
    inset_number(c, format, "indent", 18);
    inset_apply(c, field, format, "setNewTextFormat");
    inset_bool(c, field, "multiline", true);
    inset_bool(c, field, "wordWrap", true);
    inset_number(c, field, "_width", 124);
    inset_string(c, field, "text", "A V A V");
    inset_string(c, field, "autoSize", "left");
    if (!(counted_metric(c, field, "textWidth") == 90 && counted_metric(c, field, "_height") == 94))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value out;
    if (!(fvm_object_call(c, field, "getTextFormat", 0, 0, &out) == FVM_OK &&
          counted_metric(c, out.object, "indent") == 18))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    inset_number(c, format, "indent", -18);
    inset_apply(c, field, format, "setTextFormat");
    if (!(counted_metric(c, field, "textWidth") == 90 && counted_metric(c, field, "_height") == 94))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    inset_bool(c, field, "wordWrap", false);
    if (!(counted_metric(c, field, "_width") == 148 && counted_metric(c, field, "textWidth") == 114))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    inset_number(c, format, "indent", 18);
    inset_apply(c, field, format, "setTextFormat");
    if (!(counted_metric(c, field, "_width") == 184 && counted_metric(c, field, "textWidth") == 150))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
