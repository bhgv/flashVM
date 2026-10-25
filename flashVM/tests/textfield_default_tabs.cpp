/* Empty custom tables select a real font-derived periodic interval in both styles. */
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
    inset_apply(c, field, format, "setNewTextFormat");
    inset_string(c, field, "text", "A\tV\tA");
    if (!(counted_metric(c, field, "textWidth") == 199 && counted_metric(c, field, "length") == 5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    inset_string(c, field, "autoSize", "left");
    if (!(counted_metric(c, field, "_width") == 203 && counted_metric(c, field, "_height") == 48))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    inset_number(c, format, "size", 20);
    inset_apply(c, field, format, "setTextFormat");
    if (!(counted_metric(c, field, "textWidth") == 99.5 && counted_metric(c, field, "_width") == 103.5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value args[2], out;
    if (!(fvm_value_string(args, "A\tV\tA") == FVM_OK &&
          fvm_object_call(c, format, "getTextExtent", 1, args, &out) == FVM_OK &&
          counted_metric(c, out.object, "width") == 99.5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    inset_string(c, field, "text", "A\tV");
    if (!(counted_metric(c, field, "textWidth") == 111.5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    } /* future remains 40px. */
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
